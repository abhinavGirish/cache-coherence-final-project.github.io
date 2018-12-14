
#include <algorithm>

#include "simulator.hpp"

Simulator::Simulator(const char *tracefile, CacheConfig config, bool roi, int limit, bool mig, int interconnect, bool numa)
    : table(OUTSANDING_REQUESTS), roi(roi), limit(limit)
{

    //initializing task graph with tracefile, task manager
    task_graph.reset(contech::TaskGraph::initFromFile(tracefile));

    //setting task limits
    int task_limit = limit;
    int num_tasks = task_graph->getNumberOfTasks();
    if(limit == -1)
    {
        this->limit = num_tasks;
    }
    else if(limit > num_tasks)
    {
        this->limit = num_tasks;
        task_limit = -1;
    }

    this->interconnect = interconnect;

    config.nproc = task_graph->getNumberOfContexts();

    manager.reset(new TaskManager(*task_graph, roi, task_limit));

    caches.reserve(config.nproc);
    for(uint32_t i=0; i<config.nproc; i++)
    {
        caches.push_back(Cache(*manager, i, config));
    }
    std::vector<Cache *> cache_refs;
    //std::vector<Cache *> cache_refs2;
    for(auto &cache : caches)
    {
        cache_refs.push_back(&cache);
        //cache_refs2.push_back(&cache);
    }


    // adding all the coherence managers for all of the processors in the system
    coherence_managers.reserve(config.nproc);

    for(uint32_t i=0; i<config.nproc; i++)
    {
            coherence_managers.push_back(unique_ptr<CoherenceManagerInterface>(new MigratoryManager(&caches.at(i), i, config.nproc, table, mig)));
    }
    mem.reset(new MigMemory());

    // sets the coherence manager field of the cache located on each processor
    for(uint32_t i=0; i<config.nproc; i++)
    {
        caches[i].set_coherence_manager(static_cast<CoherenceManagerInterface *>
            (&*coherence_managers.at(i)));
    }

    // adding a receiver for every memory coherence manager
    std::vector<Receiver *> receivers;
    for(auto &m : coherence_managers)
    {
        receivers.push_back(dynamic_cast<Receiver*>(&*m));
    }

    receivers.push_back(dynamic_cast<Receiver*>(&*mem));

    // setting number of processors and receivers on bus
    // connecting bus to coherence managers

    if(interconnect == 1){
	if(numa)
		crossbar.set_numa();
        crossbar.set_nproc(config.nproc);
        crossbar.set_receivers(receivers);
        crossbar.init_interconnect();
        for(auto &m : coherence_managers)
        {
            m->set_crossbar(&crossbar);
        }

        crossbar.set_cache_refs(cache_refs);

        mem->set_crossbar(&crossbar);

    }
    else if(interconnect == 2){
	if(numa)
		ring.set_numa();
        ring.set_nproc(config.nproc);
        ring.set_receivers(receivers);
        ring.init_interconnect();
        for(auto &m : coherence_managers)
        {
            m->set_ring(&ring);
        }
        ring.set_cache_refs(cache_refs);
        mem->set_ring(&ring);
    }
    else if(interconnect == 3){
	if(numa)
		mesh.set_numa();
        mesh.set_nproc(config.nproc);
        mesh.set_receivers(receivers);
        mesh.init_interconnect();
        for(auto &m : coherence_managers)
        {
            m->set_mesh(&mesh);
        }
        mesh.set_cache_refs(cache_refs);
        mem->set_mesh(&mesh);

    }
    else if(interconnect == 4){
	if(numa)
		torus.set_numa();
        torus.set_nproc(config.nproc);
        torus.set_receivers(receivers);
        torus.init_interconnect();
        for(auto &m : coherence_managers)
        {
            m->set_torus(&torus);
        }
        torus.set_cache_refs(cache_refs);
        mem->set_torus(&torus);

    }
    else{
        bus.set_nproc(config.nproc);
        bus.set_receivers(receivers);

        for(auto &m : coherence_managers)
        {
            m->set_bus(&bus);
        }
        mem->set_bus(&bus);
    }
    checker.reset(static_cast<ConsistencyCheckerInterface *>(
            new MIGConsistencyChecker(cache_refs, table, config)));
}

void Simulator::run(bool progbar)
{
    // ProgressBar bar(limit);
    // uint32_t pos = -1;

    // int stop = 0;
    // bus.log_on();

    while(!done)
    {
        event();

        /*
        if(limit > 0 && progbar)
        {
            if(pos != manager->get_count())
            {
                pos = manager->get_count();
                bar.set(pos);
            }

        }
        */
        // if(stop++ > 1000)
        //     return;
        /*
        if(pos >= 700)
        {
            bus.log_on();
            manager->log_on();
            caches[0].log_on();


            std::cout << "----------------------------------------" << std::endl;
            std::cout << "Pos: " << pos << std::endl;
            for(auto &c : caches)
            {
                std::cout << c << std::endl;
            }
            for(auto &cm : coherence_managers)
            {
                cm->log_on();
                cm->print(std::cout);
                std::cout << std::endl;
            }
            // stop++;
        }
        */
        // if(pos == 708)
        // {
        //     return;
        // }

    }

    // check();
    std::cout << "num migratory: " << bus.migratory << std::endl;
}


// runs a simulator event, equivalent to running an event for all the caches
void Simulator::event()
{
    if(done)
    {
        return;
    }
    num_cycles++;

    uint32_t ndone = 0;

    // simulate an event for all the caches in the system
    for(auto &cache : caches)
    {
        if(cache.is_done())
        {
            ndone++;
            continue;
        }
        else
        {
            cache.event();
            // checker->check_requests();
        }
    }

    // update the coherence managers for all the caches
    for(auto &m : coherence_managers)
    {
        m->event();
    }

    // update the bus and memory interface
    if(interconnect==1){
        crossbar.event();
    }
    else if(interconnect ==2){
        ring.event();
    }
    else if(interconnect == 3){
        mesh.event();
    }
    else if(interconnect == 4){
	torus.event();
    }
    else{
        bus.event();
    }
    mem->event();


    if(ndone >= caches.size())
    {
        done = true;
    }

}

void Simulator::check()
{
    checker->check();
}

CacheStats Simulator::get_stats()
{
    CacheStats stats = {};
    for(auto &cache : caches)
    {
        stats += cache.get_stats();
    }

    return stats;
}

uint64_t Simulator::get_num_messages() {
    if(interconnect == 1)
        return crossbar.get_num_messages();
    else if(interconnect == 2)
        return ring.get_num_messages();
    else if(interconnect == 3)
        return mesh.get_num_messages();
    else if(interconnect == 4)
	return torus.get_num_messages();
    else
        return bus.get_num_messages();
}

uint64_t Simulator::get_contentions() {
    if(interconnect == 1)
        return crossbar.get_contentions();
    else if(interconnect == 2)
        return ring.get_contentions();
    else if(interconnect == 3)
        return mesh.get_contentions();
    else if(interconnect == 4)
	return torus.get_contentions();
    else
        return -1;
}

uint64_t Simulator::get_hops() {
    if(interconnect == 1)
        return crossbar.get_hops();
    else if(interconnect == 2)
        return ring.get_hops();
    else if(interconnect == 3)
        return mesh.get_hops();
    else if(interconnect == 4)
	return torus.get_hops();
    else
        return -1;
}


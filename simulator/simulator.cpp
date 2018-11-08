
#include <algorithm>

#include "simulator.hpp"


Simulator::Simulator(const char *tracefile, CacheConfig config, bool roi, int limit, bool mig)
    : table(OUTSANDING_REQUESTS), roi(roi), limit(limit)
{

    task_graph.reset(contech::TaskGraph::initFromFile(tracefile));

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

    config.nproc = task_graph->getNumberOfContexts();

    manager.reset(new TaskManager(*task_graph, roi, task_limit));

    caches.reserve(config.nproc);
    for(uint32_t i=0; i<config.nproc; i++)
    {
        caches.push_back(Cache(*manager, i, config));
    }
    std::vector<Cache *> cache_refs;
    for(auto &cache : caches)
    {
        cache_refs.push_back(&cache);
    }


    coherence_managers.reserve(config.nproc);
    // if(mig)
    // {
        for(uint32_t i=0; i<config.nproc; i++)
        {
            coherence_managers.push_back(unique_ptr<CoherenceManagerInterface>(new MigratoryManager(&caches.at(i), i, config.nproc, table, mig)));
        }
        mem.reset(new MigMemory());
    // }

    /*
    else
    {
        for(uint32_t i=0; i<config.nproc; i++)
        {
            coherence_managers.push_back(unique_ptr<CoherenceManagerInterface>(new MESIManager(&caches.at(i), i, config.nproc, table)));
        }
        mem.reset(new MESIMemory());
    }
    */


    for(uint32_t i=0; i<config.nproc; i++)
    {
        caches[i].set_coherence_manager(static_cast<CoherenceManagerInterface *>
            (&*coherence_managers.at(i)));
    }

    std::vector<Receiver *> receivers;
    for(auto &m : coherence_managers)
    {
        receivers.push_back(dynamic_cast<Receiver*>(&*m));
    }



    receivers.push_back(dynamic_cast<Receiver*>(&*mem));

    bus.set_nproc(config.nproc);
    bus.set_receivers(receivers);
    
    for(auto &m : coherence_managers)
    {
        m->set_bus(&bus);
    }
    mem->set_bus(&bus);

    // if(mig)
    // {
        checker.reset(static_cast<ConsistencyCheckerInterface *>(
            new MIGConsistencyChecker(cache_refs, table, config)));
    /*
    }
    else
    {
        checker.reset(static_cast<ConsistencyCheckerInterface *>(
            new MESIConsistencyChecker(cache_refs, table, config)));
    }
    */
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



void Simulator::event()
{
    if(done)
    {
        return;
    }
    num_cycles++;

    uint32_t ndone = 0;
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

    for(auto &m : coherence_managers)
    {
        m->event();
    }

    bus.event();
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

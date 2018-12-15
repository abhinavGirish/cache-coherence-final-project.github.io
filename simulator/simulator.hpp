
#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <memory>
#include <vector>
#include <cstdint>

#include "event.hpp"
#include "task_manager.hpp"
#include "cache.hpp"
#include "mesi_consistency_checker.hpp"
#include "bus.hpp"
#include "crossbar.hpp"
#include "ring.hpp"
#include "mesh.hpp"
#include "torus.hpp"
#include "omega.hpp"
#include "coherence_manager_interface.hpp"
#include "mesi_memory.hpp"
#include "mig_memory.hpp"
#include "mesi_manager.hpp"
#include "migratory_manager.hpp"
#include "progress_bar.hpp"
#include "consistency_checker_interface.hpp"
#include "mig_consistency_checker.hpp"

const int OUTSANDING_REQUESTS = 8;

class Simulator : public Event
{

private:
    unique_ptr<contech::TaskGraph> task_graph;
    unique_ptr<TaskManager> manager;

    std::vector<Cache> caches;
    std::vector<unique_ptr<CoherenceManagerInterface>> coherence_managers;
    RequestTable table;
    Bus bus;
    Crossbar crossbar;
    Ring ring;
    Mesh mesh;
    Torus torus;
    Omega omega;
    unique_ptr<MemoryInterface> mem;

    uint64_t num_cycles = 0;

    bool roi;
    int limit;
    unique_ptr<ConsistencyCheckerInterface> checker;
    int interconnect;
    bool done = false;

public:
    Simulator(const char *tracefile, CacheConfig config, bool roi, int limit, bool mig, int interconnect, bool numa, bool unidirectional, size_t pointers);

    void run(bool progbar);
    void run() { run(true); }
    void event();
    bool is_done() { return done; };
    CacheStats get_stats();
    void check();
    uint64_t get_cycles() { return num_cycles; }
    uint64_t get_num_messages();
    uint64_t get_contentions();
    uint64_t get_hops();
    bool is_roi() { return roi; }

    void output_stats(const char *out){
	if(interconnect == 1)
		crossbar.write_stats(out);
	else if(interconnect == 2)
		ring.write_stats(out);
	else if(interconnect == 3)
		mesh.write_stats(out);
	else if(interconnect == 4)
		torus.write_stats(out);
	else if(interconnect == 5)
		omega.write_stats(out);
    }
};


#endif // SIMULATOR_HPP

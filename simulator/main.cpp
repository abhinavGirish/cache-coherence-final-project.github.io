#include <memory>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include "../contech/common/taskLib/TaskGraph.hpp"
#include "boost/program_options.hpp"

#include "simulator.hpp"


const int BLOCK_BITS  = 6; // 64 bytes
const int SET_BITS    = 9;
const int NUM_LINES   = 8; // 8 lines per set
const int HIT_LATENCY = 11;


namespace po = boost::program_options;

int main(int argc, char **argv)
{
    std::string tracefile;
    std::string outfile;
    int limit;
    bool roi;
    bool mig;
    bool numa;
    bool unidirectional;
    int interconnect;
    po::positional_options_description p;
    p.add("tracefile", 1);
    size_t pointers;

    //messages for handling bad cmdline input, displaying
    //cmdline options
    try
    {
        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("tracefile", po::value<std::string>(&tracefile)->required(),
                "trace file to simulate")
	    ("outfile", po::value<std::string>(&outfile)->default_value(""),
                "output individual stats for interconnects to file")
            ("limit,l", po::value<int>(&limit)->default_value(-1),
                "limit number of tasks to simulate")
            ("roi", po::bool_switch(&roi)->default_value(false),
                "only simulate Region Of Interest")
            ("mig", po::bool_switch(&mig)->default_value(false),
                "use migratory coherence manager")
            ("interconnect",po::value<int>(&interconnect)->default_value(0),  "0 - bus, 1 - crossbar, 2 - ring, 3 - mesh, 4 - torus")
	    ("pointers", po::value<size_t>(&pointers)->default_value(0), "number of pointers to use for limited pointer scheme. 0 for full table")
	    ("numa", po::bool_switch(&numa)->default_value(false),
                "use non-uninform memory access")
	    ("unidirectional", po::bool_switch(&unidirectional)->default_value(false), "use 2 unidirectional sub-interconnects instead of 1 bidirectional sub-interconnect")
        ;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv)
            .options(desc).positional(p).run(), vm);

        if(vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);
    }
    catch(std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }


    struct stat info;
    if(stat(tracefile.c_str(), &info) !=0)
    {
        std::cerr << "Could not open tracefile" << std::endl;
        return -1;
    }


    CacheConfig config = {
        0,
        BLOCK_BITS,
        SET_BITS,
        NUM_LINES,
        HIT_LATENCY,
        HIT_LATENCY
    };

    /* run the simulation with the tracefile, cache configuration, region of interest, migratory
       flags and limit on tasks to simulate*/
    Simulator sim(tracefile.c_str(), config, roi, limit, mig, interconnect, numa,unidirectional, pointers);
    std::cout << "Initialization complete; beginning simulation" << std::endl;

    sim.run();

    std::cout << "Simulation complete" << std::endl;
    sim.check();
    if(mig)
    {
        std::cout << "Migratory" << std::endl;
    }
    std::cout << sim.get_stats();
    std::cout << "cycles: " << sim.get_cycles() << std::endl;
    std::cout << "num messages: " << sim.get_num_messages() << std::endl;
    std::cout << "contentions: " << sim.get_contentions() << std::endl;
    std::cout << "hops: " << sim.get_hops() << std::endl;
    std::cout << std::endl;

    if(outfile.compare("") != 0)
	sim.output_stats(outfile.c_str());

    return 0;
}

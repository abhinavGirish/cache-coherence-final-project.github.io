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
    int limit;
    bool roi;
    bool mig;
    po::positional_options_description p;
    p.add("tracefile", 1);

    try
    {
        po::options_description desc("Options");
        desc.add_options()
            ("help", "produce help message")
            ("tracefile", po::value<std::string>(&tracefile)->required(), 
                "trace file to simulate")
            ("limit,l", po::value<int>(&limit)->default_value(-1),
                "limit number of tasks to simulate")
            ("roi", po::bool_switch(&roi)->default_value(false),
                "only simulate Region Of Interest")
            ("mig", po::bool_switch(&mig)->default_value(false),
                "use migratory coherence manager")
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
        std::cerr << "Could not open tracefle" << std::endl;
        return -1;
    }
    /*
    int limit = -1;
    if(argc >= 3)
    {
        limit = std::stoi(argv[2]);
    }
    */

    CacheConfig config = {
        0, 
        BLOCK_BITS,
        SET_BITS,
        NUM_LINES,
        HIT_LATENCY,
        HIT_LATENCY
    };
    Simulator sim(tracefile.c_str(), config, roi, limit, mig);
    std::cout << "Initialization complete; begining simulation" << std::endl;

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
    std::cout << std::endl;
    
    return 0;
}

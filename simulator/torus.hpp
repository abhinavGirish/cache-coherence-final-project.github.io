
#ifndef TORUS_HPP
#define TORUS_HPP

#include <vector>
#include <queue>
#include <map>
#include <math.h>

#include "event.hpp"
#include "countdown.hpp"
#include "bus.hpp"
#include "cache.hpp"
#include "crossbar.hpp"

class Torus : public Event
{
private:
    std::vector<Receiver*> receivers;
    std::vector<subinterconnect> interconnects;
    uint64_t num_messages = 0;
    uint64_t contentions = 0;
    uint64_t length = 0;
    uint64_t hops = 0;
    uint32_t nproc;
    std::vector<Cache *> cache_refs;
    std::map<uint64_t,uint64_t> directory;

    bool logging = false;

public:
    Torus() {} ;
    Torus(std::vector<Receiver*> receivers) : receivers(receivers) {}

    void init_interconnect(){ interconnects.resize(nproc+1); }

    void set_nproc(uint32_t nproc) { this->nproc = nproc; length = (uint64_t)sqrt((double)nproc); }
    void set_receivers(std::vector<Receiver*> receivers) { this->receivers = receivers; }
    uint64_t get_num_messages(){ return num_messages; }
    uint64_t get_contentions(){ return contentions; }
    uint64_t get_hops(){ return hops; }
    void set_cache_refs(std::vector<Cache *> cache_refs){this->cache_refs = cache_refs;}

    uint64_t get_directory_info(uint64_t addr){ return directory[addr];}
    bool is_set(uint64_t addr, uint32_t proc){return directory[addr]>>proc & 0x1;}
    void add_to_directory(uint64_t addr, uint32_t proc){ directory[addr] = directory[addr] | (0x1<<proc); }
    void set_new_directory(uint64_t addr, uint32_t proc){ directory[addr] = 0x1 << proc;}
    void event();
    void broadcast(CMsg msg);
    void send(CMsg msg);
    void send_ack(CMsg msg);
    void log_on() { logging = true; }
    void log_off() { logging = false; }

    int migratory = 0;

    bool check_directory(uint64_t addr);
    uint32_t num_proc(uint64_t addr);
};



#endif // BUS_HPP

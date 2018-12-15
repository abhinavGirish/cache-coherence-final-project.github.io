
#ifndef CROSSBAR_HPP
#define CROSSBAR_HPP

#include <vector>
#include <queue>
#include <map>

#include "event.hpp"
#include "countdown.hpp"
#include "bus.hpp"
#include "cache.hpp"

/*
enum class CMsgType
{
    none,
    busRd,
    busRdX,
    ack,
    data
};
std::ostream &operator<<(std::ostream& os, const CMsgType &type);

struct CMsg
{
    CMsgType type;
    uint32_t sender;    // For BusRd, BusRdX
    uint32_t receiver;  // For ack
    uint64_t addr;
    size_t flags;
};
std::ostream &operator<<(std::ostream &os, const CMsg &msg);


class Receiver
{
public:
    virtual void receive(CMsg msg) = 0;
    virtual ~Receiver() {};
};


const int BUS_DLY = 10;
*/

struct subinterconnect
{
    std::queue<CMsg> incomming;
    std::queue<bool> flip;
    uint64_t hops = 0;
    uint64_t contentions = 0;
    Countdown delay;
};

class Crossbar : public Event
{
private:
    std::vector<Receiver*> receivers;
    //std::queue<CMsg> incomming;
    std::vector<subinterconnect> interconnects;
    uint64_t num_messages = 0;
    uint32_t nproc;
    uint64_t contentions = 0;
    uint64_t hops = 0;
    std::vector<Cache *> cache_refs;
    std::map<uint64_t,uint64_t> directory;
    bool numa = false;
    bool unidirectional = false;
    //Countdown delay;
    //Countdown acks;
    bool logging = false;

public:
    Crossbar() {} ;
    Crossbar(std::vector<Receiver*> receivers) : receivers(receivers) {}

    void init_interconnect(){ interconnects.resize((nproc+1)*(nproc+1)); }

    void set_numa(){numa=true;}
    void set_unidirectional(){ unidirectional= true; }
    void set_nproc(uint32_t nproc) { if(numa){this->nproc = nproc-1;} else{this->nproc = nproc; }}
    void set_receivers(std::vector<Receiver*> receivers) { this->receivers = receivers; }
    uint64_t get_num_messages(){ return num_messages; }
    uint64_t get_contentions(){ return contentions; }
    uint64_t get_hops(){ return hops; }
    void set_cache_refs(std::vector<Cache *> cache_refs){this->cache_refs = cache_refs;}

    uint64_t get_directory_info(uint64_t addr){ return directory[addr];}
    bool is_set(uint64_t addr, uint32_t proc){return directory[addr]>>proc & 0x1;}
    void add_to_directory(uint64_t addr, uint32_t proc){ directory[addr] = directory[addr] | (0x1<<proc); }
    void set_new_directory(uint64_t addr, uint32_t proc){ directory[addr] = 0x1 << proc;}

    uint32_t get_addr_proc(uint64_t addr){return (addr>>12)%(nproc+1); }

    void event();
    void broadcast(CMsg msg);
    void send(CMsg msg);
    void send_ack(CMsg msg);
    void log_on() { logging = true; }
    void log_off() { logging = false; }


    void write_stats(const char* outfile);

    int migratory = 0;

    bool check_directory(uint64_t addr);
    uint32_t num_proc(uint64_t addr);
};



#endif // BUS_HPP

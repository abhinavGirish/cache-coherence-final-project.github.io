

#ifndef MIGRATORY_MANAGER_HPP
#define MIGRATORY_MANAGER_HPP

#include <queue>
#include <iostream>

#include "cache.hpp"
#include "event.hpp"
#include "bus.hpp"
#include "crossbar.hpp"
#include "ring.hpp"
#include "coherence_manager_interface.hpp"
#include "coherence_manager_misc.hpp"
#include "mig_memory.hpp"
#include "migratory_const.hpp"
#include "request_table.hpp"

// class ConsistencyChecker;





class MigratoryManager :  public Receiver, public CoherenceManagerInterface
{
public:

    struct PendingRequest
    {
        CMsgType type;
        uint64_t addr;
        uint32_t needed_acks;
        int tag;
        bool got_data;
        bool shared;
        bool migratory;
    };


    // friend ConsistencyChecker;
    friend std::ostream &operator<<(std::ostream &os, const MigratoryManager &cm);


private:
    Cache *cache;
    uint32_t proc;
    uint32_t nproc;

    int counter = 0;

    Bus *bus;
    Crossbar *crossbar;
    Ring *ring;
    int interconnect;

    std::queue<CMsg> incomming;
    std::queue<CMsg> incomming_temp;
    bool swap;
    RequestTable &request_table;

    PendingRequest pending_request;
    bool pending_reissue = false;

    bool do_mig;
    bool logging = false;

    void recv_busRd(CMsg &msg);
    void recv_busRdX(CMsg &msg);
    void recv_ack(CMsg &msg);
    void recv_data(CMsg &msg);

public:
    MigratoryManager(Cache *cache, uint32_t proc, uint32_t nproc, RequestTable &request_table, bool do_mig);

    void set_bus(Bus *bus) { interconnect = 0; this->bus = bus; }
    void set_crossbar(Crossbar *crossbar) { interconnect = 1; this->crossbar = crossbar; }
    void set_ring(Ring *ring){interconnect = 2; this->ring = ring;}
    void event();
    void receive(CMsg msg) { incomming.push(msg); }

    void handle_incomming();
    void handle_pending_request();

    void read(uint64_t addr);
    void write(uint64_t addr);

    bool can_read(size_t state);
    bool can_write(size_t state);
    bool rw_upgrade(size_t state);
    size_t rw_upgrade_state(size_t state);

    void print(ostream &os);


    void log_on() { logging = true; }
    void log_off() { logging = false; }
};









#endif // MIGRATORY_MANAGER_HPP

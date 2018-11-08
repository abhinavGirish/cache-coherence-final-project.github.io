

#ifndef COHERENCE_MANAGER_HPP
#define COHERENCE_MANAGER_HPP

#include <queue>
#include <iostream>

#include "cache.hpp"
#include "event.hpp"
#include "bus.hpp"
#include "coherence_manager_interface.hpp"
#include "coherence_manager_misc.hpp"
#include "mesi_memory.hpp"
#include "mesi_const.hpp"
#include "request_table.hpp"






class MESIManager : public Receiver, public CoherenceManagerInterface
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
    };

    friend std::ostream &operator<<(std::ostream &os, const MESIManager &cm);


private:
    Cache *cache;
    uint32_t proc;
    uint32_t nproc;

    Bus *bus;
    std::queue<CMsg> incomming;
    RequestTable &request_table;

    PendingRequest pending_request;
    bool pending_reissue = false;

    bool logging = false;

public:
    MESIManager(Cache *cache, uint32_t proc, uint32_t nproc, RequestTable &request_table);

    void set_bus(Bus *bus) { this->bus = bus; }


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









#endif // COHERENCE_MANAGER_HPP


#ifndef MESI_MEMORY_HPP
#define MESI_MEMORY_HPP

#include <map>
#include <queue>
#include <cassert>
#include <climits>
#include <iostream>

#include "crossbar.hpp"
#include "bus.hpp"
#include "countdown.hpp"
#include "event.hpp"
#include "mesi_const.hpp"
#include "memory_interface.hpp"


class MESIMemory : public MemoryInterface
{
private:

    struct PendingMemOp
    {
        Countdown count;
        uint32_t requester;
    };


    uint32_t latency = 100;
    Bus *bus;
    std::queue<CMsg> incomming;
    std::map<uint64_t, PendingMemOp> table;

public:
    MESIMemory() {}
    MESIMemory(uint32_t latency) : latency(latency) {}
    MESIMemory(uint32_t latency, Bus *bus) : latency(latency), bus(bus) { }

    void set_latency(uint32_t latency) { this->latency = latency; }
    void set_bus(Bus *bus) { this->bus = bus; }
    void receive(CMsg msg) { incomming.push(msg); }
    void event();
};




#endif // MESI_MEMORY_HPP

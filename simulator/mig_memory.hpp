
#ifndef MIG_MEMORY_HPP
#define MIG_MEMORY_HPP

#include <map>
#include <queue>
#include <cassert>
#include <climits>

#include "bus.hpp"
#include "crossbar.hpp"
#include "ring.hpp"
#include "mesh.hpp"
#include "countdown.hpp"
#include "event.hpp"
#include "mig_const.hpp"
#include "memory_interface.hpp"



class MigMemory : public MemoryInterface
{
private:
    struct PendingMemOp
    {
        Countdown count;
        uint32_t requester;
    };


    uint32_t latency = 100;
    Bus *bus;
    Crossbar *crossbar;
    Ring *ring;
    Mesh *mesh;
    int interconnect;
    std::queue<CMsg> incomming;
    std::map<uint64_t, PendingMemOp> table;

public:
    MigMemory() {}
    MigMemory(uint32_t latency) : latency(latency) {}
    MigMemory(uint32_t latency, Bus *bus) : latency(latency), bus(bus) { }

    void set_latency(uint32_t latency) { this->latency = latency; }
    void set_bus(Bus *bus) {interconnect = 0; this->bus = bus;}
    void set_crossbar(Crossbar *crossbar) {interconnect = 1; this->crossbar = crossbar;}
    void set_ring(Ring *ring) {interconnect = 2; this->ring = ring;}
    void set_mesh(Mesh *mesh) {interconnect = 3, this->mesh = mesh;}
    void receive(CMsg msg) { incomming.push(msg); }
    void event();
};




#endif // MIG_MEMORY_HPP

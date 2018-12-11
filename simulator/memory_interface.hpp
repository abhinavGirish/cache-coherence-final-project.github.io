
#ifndef MEMORY_INTERFACE_HPP
#define MEMORY_INTERFACE_HPP

#include <iostream>

const int MEM = INT_MAX;


class MemoryInterface : public Event, public Receiver
{
public:
    virtual void set_latency(uint32_t latency) = 0;
    virtual void set_bus(Bus *bus) = 0;
    virtual void set_crossbar(Crossbar *crossbar) = 0;
    virtual void set_ring(Ring *ring) = 0;
    virtual void set_mesh(Mesh *mesh) = 0;
    virtual void receive(CMsg msg) = 0;
    virtual void event() = 0;
};





#endif // MEMORY_INTERFACE_HPP

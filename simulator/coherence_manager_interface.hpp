
#ifndef COHERENCE_MANAGER_INTERFACE_HPP
#define COHERENCE_MANAGER_INTERFACE_HPP

#include <string>

class Bus;

class CoherenceManagerInterface : public Event
{
private:

public:
    virtual ~CoherenceManagerInterface(){};
    virtual void read(uint64_t addr) = 0;
    virtual void write(uint64_t addr) = 0;

    virtual bool can_read(size_t state) = 0;
    virtual bool can_write(size_t state) = 0;

    virtual bool rw_upgrade(size_t state) = 0;
    virtual size_t rw_upgrade_state(size_t state) = 0;

    virtual void set_bus(Bus *bus) = 0;

    virtual void event() = 0;

    virtual void print(ostream &os) = 0;    

    virtual void log_on() = 0;
    virtual void log_off() = 0;
};








#endif // COHERENCE_MANAGER_INTERFACE_HPP



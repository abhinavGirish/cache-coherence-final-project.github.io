
#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include <queue>

#include "event.hpp"
#include "countdown.hpp"

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

class Bus : public Event
{
private:
    std::vector<Receiver*> receivers;
    std::queue<CMsg> incomming;
    uint64_t num_messages;
    uint32_t nproc;

    Countdown delay;
    Countdown acks;
    bool logging = false;

public:
    Bus() {} ;
    Bus(std::vector<Receiver*> receivers) : receivers(receivers) {}

    void set_nproc(uint32_t nproc) { this->nproc = nproc; }
    void set_receivers(std::vector<Receiver*> receivers) { this->receivers = receivers; }
    uint64_t get_num_messages(){ return num_messages; }

    void event();
    void broadcast(CMsg msg);
    void send_ack(CMsg msg);
    void log_on() { logging = true; }
    void log_off() { logging = false; }

    int migratory = 0;
};



#endif // BUS_HPP
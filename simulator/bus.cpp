
#include <ostream>
#include <iostream>

#include "bus.hpp"


////////////////////////////////////////////////////////////////////////////////
// CMsg
////////////////////////////////////////////////////////////////////////////////


std::ostream &operator<<(std::ostream& os, const CMsgType &type)
{
    switch(type)
    {
        case CMsgType::none:
            os << "CMsgType::none";
            break;
        case CMsgType::busRd:
            os << "CMsgType::busRd";
            break;
        case CMsgType::busRdX:
            os << "CMsgType::busRdX";
            break;
        case CMsgType::ack:
            os << "CMsgType::ack";
            break;
        case CMsgType::data:
            os << "CMsgType::data";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const CMsg &msg)
{
    os << "CMsg {type: " << msg.type << ", sender: " << msg.sender
        << ", receiver: " << msg.receiver << ", addr: 0x" << std::hex
        << msg.addr << std::dec << ", flags: " << std::hex << msg.flags
        << std::dec << " }";
    return os;
}




////////////////////////////////////////////////////////////////////////////////
// Bus
////////////////////////////////////////////////////////////////////////////////


void Bus::broadcast(CMsg msg)
{
    num_messages++;
    // std::cout << "bus got " << msg << std::endl;
    incomming.push(msg);
}

void Bus::send_ack(CMsg msg)
{
    if(msg.flags & 0x2)
    {
        migratory++;
    }
    if(logging)
    {
        std::cout << "bus ack fast path " << msg << std::endl;
    }
    acks.tick();
    receivers[msg.receiver]->receive(msg);
    receivers[nproc]->receive(msg);
}

void Bus::event()
{
    if(delay.is_done())
    {
        if(acks.is_done())
        {
            if(incomming.size() > 0)
            {
                if(logging)
                {
                    std::cout << "bus reseting delay for next" << std::endl;
                }
                delay.reset(BUS_DLY);
            }
            else
            {
                if(logging)
                {
                    std::cout << "bus waiting" << std::endl;
                }
            }
        }
        else
        {
            if(logging)
            {
                std::cout << "bus waiting for acks: " << acks.get_remaining() << std::endl;
            }
        }
    }
    else
    {

        if(delay.tick())
        {
            if(logging)
            {
                std::cout << "bus delaying done" << std::endl;
            }
            CMsg msg = incomming.front();
            incomming.pop();
            for(auto recv : receivers)
            {
                recv->receive(msg);
            }

            if(logging)
            {
                std::cout << "bus broadcasting " << msg << std::endl;
            }

            if(msg.type == CMsgType::busRd || msg.type == CMsgType::busRdX)
            {
                if(logging)
                {
                    std::cout << "bus waiting for acks" << std::endl;
                }
                acks.reset(nproc-1);
            }

        }
        else
        {
            if(logging)
            {
                std::cout << "bus delay tick" << std::endl;
            }
        }
    }
}

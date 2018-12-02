
#include <ostream>
#include <iostream>

#include "crossbar.hpp"
#include "bus.hpp"

////////////////////////////////////////////////////////////////////////////////
// CMsg
////////////////////////////////////////////////////////////////////////////////

/*
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


*/

////////////////////////////////////////////////////////////////////////////////
// Crossbar
////////////////////////////////////////////////////////////////////////////////


void Crossbar::broadcast(CMsg msg)
{
    num_messages++;
    // std::cout << "bus got " << msg << std::endl;
    assert(check_directory(msg.addr));
    incomming.push(msg);
}

void Crossbar::send_ack(CMsg msg)
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
    assert(check_directory(msg.addr));
    receivers[msg.receiver]->receive(msg);
    receivers[nproc]->receive(msg);
}

void Crossbar::event()
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
            assert(check_directory(msg.addr));
            /*if(msg.type == CMsgType::busRd){
                add_to_directory(msg.addr,msg.sender);
            }
            else if(msg.type == CMsgType::busRdX){
                set_new_directory(msg.addr,msg.sender);
            }*/
            incomming.pop();
            /*for(auto recv : receivers)
            {
                recv->receive(msg);
            }*/
            size_t acks_needed = 0;
            if(msg.type == CMsgType::busRd || msg.type == CMsgType::busRdX){
                receivers[nproc]->receive(msg);
                uint64_t current_state = get_directory_info(msg.addr);
                for(size_t i=0;i<nproc;i++){
                    if(current_state & 0x1 && i!=msg.sender){
                        receivers[i]->receive(msg);
                        acks_needed++;
                    }
                    current_state = current_state>>1;
                }
            }
            else{
                assert(msg.receiver < nproc);
                receivers[msg.receiver]->receive(msg);
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
                acks.reset(acks_needed);
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


bool Crossbar::check_directory(uint64_t addr){
    uint64_t current_state = get_directory_info(addr);
    for(size_t i=0;i<nproc;i++){
        Line *line;
        bool found = cache_refs[i]->find_line(addr,&line);
        if(found && !((current_state>>i) & 0x1))
            return false;
        //if(!found && ((current_state>> i) & 0x1))
        //    return false;
    }
    return true;
}

uint32_t Crossbar::num_proc(uint64_t addr){
    uint64_t num = get_directory_info(addr);
    uint32_t ans = 0;
    while(num != 0){
        if(num & 0x1)
            ans++;
        num = num >> 1;
    }
    return ans;
}

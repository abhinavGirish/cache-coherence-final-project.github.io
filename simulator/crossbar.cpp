
#include <ostream>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>

#include "crossbar.hpp"
#include "bus.hpp"
#include "mig_const.hpp"
#include "mig_memory.hpp"


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
    //assert(check_directory(msg.addr));
    if(msg.type == CMsgType::busRd || msg.type == CMsgType::busRdX){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        send(msg);//receivers[nproc]->receive(msg);
        uint64_t current_state = get_directory_info(msg.addr);
	bool broadcasting = broadcast_needed(msg.addr);
        size_t iter = nproc;
	if(numa)
		iter++;
	for(size_t i=0;i<iter;i++){
            if((current_state & 0x1 || broadcasting) && i!=msg.sender){
                msg.receiver = i;
                send(msg);//receivers[i]->receive(msg);
       	    }
            current_state = current_state>>1;
        }
	
    }
    else{
        if(msg.sender==MEM){
	    if(numa)
		msg.sender= nproc+1;
	    else
            	msg.sender= nproc;
	}
        send(msg);
    }
}

void Crossbar::send(CMsg msg){
    int recv = msg.receiver;
    int sent = msg.sender;
    if(numa && msg.receiver == nproc+1)
	recv = get_addr_proc(msg.addr);
    if(numa && msg.sender == nproc+1)
	sent = get_addr_proc(msg.addr);
    num_messages++;
    size_t index;
    if(unidirectional)
	index = sent*(nproc+1) + recv;
    else
	index = std::min(sent,recv)*(nproc+1) + std::max(sent,recv);
    if(interconnects[index].incomming.size()>0){
	interconnects[index].contentions++;
        contentions++;
    }
    interconnects[index].incomming.push(msg);
}

void Crossbar::send_ack(CMsg msg)
{
    if(msg.flags & 0x2)
    {
        migratory++;
    }
    //acks.tick();
    assert(check_directory(msg.addr));
    broadcast(msg);//incomming.push(msg);
    if(msg.flags & MigBusFlag::TRANSMIT){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        broadcast(msg);//incomming.push(msg);
    }
    //receivers[msg.receiver]->receive(msg);
    //receivers[nproc]->receive(msg);
}

void Crossbar::event()
{
  for(size_t i=0;i<(nproc+1)*(nproc+1);i++){
    if(interconnects[i].delay.is_done())
    {
            if(interconnects[i].incomming.size() > 0)
            {
                interconnects[i].delay.reset(BUS_DLY);
            }
    }
    else
    {
        if(interconnects[i].delay.tick())
        {
            CMsg msg = interconnects[i].incomming.front();
            //assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
            //assert(msg.receiver <= nproc);
	    interconnects[i].hops++;
	    hops++;
            receivers[msg.receiver]->receive(msg);
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

bool Crossbar::broadcast_needed(uint64_t addr){
	size_t count = 0;
	uint64_t current_state = get_directory_info(addr);
        size_t iter = nproc;
	if(numa)
		iter++;
	for(size_t i=0;i<iter;i++){
            if(current_state & 0x1){
		count++;
       	    }
	    if(count>limited_pointers)
		return true;
            current_state = current_state>>1;
        }
	return false;
}

void Crossbar::write_stats(const char *outfile){
	ofstream out;
	out.open(outfile);
	out << "sender receiver hops contentions\n";
	for(size_t i=0;i<(nproc+1)*(nproc+1);i++){
		uint64_t h = interconnects[i].hops;
		uint64_t c = interconnects[i].contentions;
		uint32_t s = i/(nproc+1);
		uint32_t r = i%(nproc+1);
		out << std::to_string(s) << " " << std::to_string(r) << " " << std::to_string(h) << " " << std::to_string(c) << "\n";
	}
	out.close();
}

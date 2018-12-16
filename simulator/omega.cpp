
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
// Omega
////////////////////////////////////////////////////////////////////////////////


void Omega::broadcast(CMsg msg)
{
    //assert(check_directory(msg.addr));
    if(msg.type == CMsgType::busRd || msg.type == CMsgType::busRdX){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        send(msg);
        uint64_t current_state = get_directory_info(msg.addr);
	bool broadcasting = broadcast_needed(msg.addr);
	size_t iter = nproc;
	if(numa)
		iter++;
        for(size_t i=0;i<iter;i++){
            if((current_state & 0x1 || broadcasting) && i!=msg.sender){
                msg.receiver = i;
                send(msg);
            }
        current_state = current_state>>1;
        }
    }
    else{
        if(msg.sender==MEM){
	    if(numa)
		msg.sender = nproc+1;
	    else
            	msg.sender=nproc;
	}
        send(msg);
    }
}

void Omega::send(CMsg msg){
    uint32_t sent = msg.sender;
    if(numa && msg.sender == nproc + 1)
	sent = get_addr_proc(msg.addr);
    num_messages++;
    uint32_t recv = next(sent,msg);
    size_t index;
    if(unidirectional)
	index = sent*(nproc+1+4*num_bits) + recv;
    else{
	index = std::min(sent,recv)*(nproc+1+4*num_bits) + std::max(sent,recv); 
	if(recv<sent)
		interconnects[index].flip.push(true);
	else
		interconnects[index].flip.push(false);
    }
    if(interconnects[index].incomming.size()>0){

    	//std::cout << sent << "  " << recv << " " << index<< " " << interconnects[index].incomming.size()  << std::endl;
	interconnects[index].contentions++;
        contentions++;
    }
    interconnects[index].incomming.push(msg);
}

void Omega::send_ack(CMsg msg)
{
    if(msg.flags & 0x2)
    {
        migratory++;
    }
    //assert(check_directory(msg.addr));
    broadcast(msg);
    if(msg.flags & MigBusFlag::TRANSMIT){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        broadcast(msg);
    }
}

void Omega::event()
{
  for(size_t i=0;i<(nproc+1+4*num_bits)*(nproc+1+4*num_bits);i++){
    uint32_t s = i/(nproc+1+4*num_bits);
    uint32_t r = i%(nproc+1+4*num_bits);
    
    if(interconnects[i].delay.is_done())
    {
            if(interconnects[i].incomming.size() > 0)
            {
                interconnects[i].delay.reset(BUS_DLY);
            }
    }
    else
    {
        if(interconnects[i].delay.tick() || s == r)
        {
	    hops++;
	    interconnects[i].hops++;
	    CMsg msg = interconnects[i].incomming.front();
            //assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
	    if(!unidirectional){
	    bool flipped = interconnects[i].flip.front();
	    if(flipped){
		uint32_t t = s;
		s = r;
		r = t;
	    }
	    interconnects[i].flip.pop();}
	    size_t recv = msg.receiver;
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
	    if(r == recv)
		receivers[msg.receiver]->receive(msg);
	    else{
		uint32_t n = next(r,msg);
		size_t index;
    		if(unidirectional)
			index = r*(nproc+1+4*num_bits) + n;
    		else{
			index = std::min(r,n)*(nproc+1+4*num_bits) + std::max(r,n); 
			if(n<r)
				interconnects[index].flip.push(true);
			else
				interconnects[index].flip.push(false);
		}
		if(interconnects[index].incomming.size()>0){
                       contentions++;
		       interconnects[index].contentions++;

		       //std::cout << r << "  " << n << " " << index << " " << interconnects[index].incomming.size() << std::endl;
		}
                interconnects[index].incomming.push(msg);

	    }

        }
    }
  }
}


bool Omega::check_directory(uint64_t addr){
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

uint32_t Omega::next(uint32_t current, CMsg msg){
	size_t recv = msg.receiver;
	if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
	size_t source = msg.sender;
	if(current<nproc+1){
		size_t s = source&0x1;
		size_t r = recv&0x1;
		if(s==0 && r==0)
			return nproc+1;
		if(s==0 && r==1)
			return nproc+2;
		if(s==1 && r==0)
			return nproc+3;
		if(s==1 && r==1)
			return nproc+4;
		assert(false);
	}
	size_t index = current-(nproc+1);
	size_t layers = index/4+1;
	if(layers==num_bits)
		return recv;
	source=source>>layers;
	recv = recv>>layers;
	size_t s = source&0x1;
	size_t r = recv&0x1;
	if(s==0 && r==0)
		return nproc+4*layers+1;
	if(s==0 && r==1)
		return nproc+4*layers+2;
	if(s==1 && r==0)
		return nproc+4*layers+3;
	if(s==1 && r==1)
		return nproc+4*layers+4;
	assert(false);

}

uint32_t Omega::num_proc(uint64_t addr){
    uint64_t num = get_directory_info(addr);
    uint32_t ans = 0;
    while(num != 0){
        if(num & 0x1)
            ans++;
        num = num >> 1;
    }
    return ans;
}

bool Omega::broadcast_needed(uint64_t addr){
	if(limited_pointers==0)
		return false;
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

void Omega::write_stats(const char *outfile){
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

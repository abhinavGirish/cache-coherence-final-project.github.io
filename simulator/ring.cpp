
#include <ostream>
#include <iostream>

#include "crossbar.hpp"
#include "bus.hpp"
#include "mig_const.hpp"
#include "mig_memory.hpp"

////////////////////////////////////////////////////////////////////////////////
// Ring
////////////////////////////////////////////////////////////////////////////////


void Ring::broadcast(CMsg msg)
{
    //assert(check_directory(msg.addr));
    if(msg.type == CMsgType::busRd || msg.type == CMsgType::busRdX){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        send(msg);
        uint64_t current_state = get_directory_info(msg.addr);
	size_t iter = nproc;
	if(numa)
		iter++;
        for(size_t i=0;i<iter;i++){
            if(current_state & 0x1 && i!=msg.sender){
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

void Ring::send(CMsg msg){
    int sent = msg.sender;
    if(numa && msg.sender == nproc + 1)
	sent = get_addr_proc(msg.addr);
    num_messages++;
    uint32_t recv = next(sent,msg); 
    if(interconnects[sent*(nproc+1) + recv].incomming.size()>0)
        contentions++;
    interconnects[sent*(nproc+1) + recv].incomming.push(msg);
}

void Ring::send_ack(CMsg msg)
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

void Ring::event()
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
	    hops++;
            uint32_t s = i/(nproc+1);
	    uint32_t r = i%(nproc+1);
            assert((r == 0 && s == nproc) || (r==nproc && s == 0) || (r>s && r-s <= 1) || (s>r && s-r <= 1));
	    CMsg msg = interconnects[i].incomming.front();
            //assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
	    size_t recv = msg.receiver;
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
	    if(r == recv)
		receivers[msg.receiver]->receive(msg);
	    else{
		uint32_t n = next(r,msg);
		if(interconnects[r*(nproc+1)+n].incomming.size()>0)
                       contentions++;
                interconnects[r*(nproc+1)+n].incomming.push(msg);

	    }

        }
    }
  }
}


bool Ring::check_directory(uint64_t addr){
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

uint32_t Ring::next(uint32_t current, CMsg msg){
	    size_t recv = msg.receiver;
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
            int index = current;
            int left_index;
            int right_index;
            if(index-1<0){
                left_index = nproc;
            }
            else{
                left_index = index -1;
            }
            if(index+1>nproc){
                right_index = 0;
            }
            else{
                right_index = index + 1;
            }
            if(left_index == recv)
		return left_index;
	    else if(right_index == recv)
                return right_index;
            else{
                int right_distance;
                int left_distance;
                if(index < recv){
                    right_distance = recv - index;
                    left_distance = index + (nproc+1 - recv);
                }
                else{
                    left_distance = index - recv;
                    right_distance = recv + (nproc+1 - index);
                }
                if(left_distance < right_distance){
                    //if(interconnects[left_index].incomming.size()>0)
                    //    contentions++;
                    //interconnects[left_index].incomming.push(msg);
		    return left_index;
                }
                else if(left_distance > right_distance){
                    //if(interconnects[right_index].incomming.size()>0)
                    //    contentions++;
                    //interconnects[right_index].incomming.push(msg);
		    return right_index;
                }
                else{
                    if(interconnects[left_index].incomming.size()<interconnects[right_index].incomming.size()){
                        //if(interconnects[left_index].incomming.size()>0)
                        //    contentions++;
                        //interconnects[left_index].incomming.push(msg);
			return left_index;
                    }
                    else{
                        //if(interconnects[right_index].incomming.size()>0)
                        //    contentions++;
                        //interconnects[right_index].incomming.push(msg);
			return right_index;
                    }
                }
            }
}

uint32_t Ring::num_proc(uint64_t addr){
    uint64_t num = get_directory_info(addr);
    uint32_t ans = 0;
    while(num != 0){
        if(num & 0x1)
            ans++;
        num = num >> 1;
    }
    return ans;
}

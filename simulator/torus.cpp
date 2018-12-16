
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
// Torus
////////////////////////////////////////////////////////////////////////////////


void Torus::broadcast(CMsg msg)
{
    assert(check_directory(msg.addr));
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
        if(msg.sender==MEM)
	{
	    if(numa)
		msg.sender = nproc+1;
	    else
            	msg.sender=nproc;
        }
	send(msg);
    }
}

void Torus::send(CMsg msg){
    uint32_t sent = msg.sender;
    if(numa && msg.sender == nproc+1)
	sent = get_addr_proc(msg.addr);
    num_messages++;
    uint32_t recv = next(sent,msg);
    //uint32_t recv = next(sent,msg);
    size_t index;
    if(unidirectional)
	index = sent*(nproc+1) + recv;
    else{
	index = std::min(sent,recv)*(nproc+1) + std::max(sent,recv); 
	if(recv<sent)
		interconnects[index].flip.push(true);
	else
		interconnects[index].flip.push(false);
    }
    if(interconnects[index].incomming.size()>0){
	interconnects[index].contentions++;
        contentions++;
    }
    interconnects[index].incomming.push(msg);
}

void Torus::send_ack(CMsg msg)
{
    if(msg.flags & 0x2)
    {
        migratory++;
    }
    assert(check_directory(msg.addr));
    broadcast(msg);
    if(msg.flags & MigBusFlag::TRANSMIT){
	if(numa)
		msg.receiver = nproc+1;
	else
        	msg.receiver = nproc;
        broadcast(msg);
    }
}

void Torus::event()
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
	    interconnects[i].hops++;
            CMsg msg = interconnects[i].incomming.front();
            assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
            uint32_t s = i/(nproc+1);
	    uint32_t r = i%(nproc+1);
	    size_t recv = msg.receiver;
	    if(!unidirectional){
	    bool flipped = interconnects[i].flip.front();
	    if(flipped){
		uint32_t t = s;
		s = r;
		r = t;
	    }
	    interconnects[i].flip.pop();}
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
	    if(r == recv)
		receivers[msg.receiver]->receive(msg);
	    else{
		uint32_t n = next(r,msg);
		size_t index;
    		if(unidirectional)
			index = r*(nproc+1) + n;
    		else{
			index = std::min(r,n)*(nproc+1) + std::max(r,n); 
			if(n<r)
				interconnects[index].flip.push(true);
			else
				interconnects[index].flip.push(false);
		}
		if(interconnects[index].incomming.size()>0){
                       contentions++;
		       interconnects[index].contentions++;
		}
                interconnects[index].incomming.push(msg);


	    } 
        }
    }
  }
}


bool Torus::check_directory(uint64_t addr){
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

uint32_t Torus::next(uint32_t current, CMsg msg){
	    int c_x = current%length;
            int c_y = current/length;
	    size_t recv = msg.receiver;
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
            int e_x = recv%length;
            int e_y = recv/length;
            int n_x = c_x;
            int n_y = c_y;
            if(c_x<e_x){
		int right_distance = e_x - c_x;
		int left_distance = c_x + (length - e_x);
		if(right_distance<left_distance){
			n_x++;
		}
		else{
			n_x--;
			if(n_x<0){
				n_x = length-1;
				while(c_y*length + n_x> nproc)
					n_x--;	
			}
		}	
            }
            else if(c_x>e_x){
		int left_distance = c_x - e_x;
		int right_distance = e_x + (length - c_x);
		if(left_distance<right_distance){
                	n_x--;
		}
		else{
			n_x++;
			if(c_y*length + n_x> nproc)
				n_x=0;
		}
	    }
            if(c_y<e_y){
                int right_distance = e_y - c_y;
		int left_distance = c_y + (length - e_y);
		if(right_distance<left_distance){
			n_y++;
		}
		else{
			n_y--;
			if(n_y<0)
				n_y = length-1;
			if(n_y*length + c_x> nproc)
				n_y--;
		}
	    }
            else if(c_y>e_y){
                int left_distance = c_y - e_y;
		int right_distance = e_y + (length - c_y);
		if(left_distance<right_distance){
                	n_y--;
		}
		else{
			n_y++;
			if(n_y*length + c_x> nproc)
				n_y=0;
		}
	    }
            if(n_x == e_x && n_y == e_y){
                //assert(msg.receiver <= nproc);
                //receivers[msg.receiver]->receive(msg);
		return e_y*length + e_x;
            }
            else{
                size_t h_index = c_y*length + n_x;
                size_t v_index = n_y*length + c_x;
                int h_size = -1;
                int v_size = -1;
                if(h_index != current && h_index < (nproc+1))
                    h_size = interconnects[h_index].incomming.size();
                if(v_index != current && v_index < (nproc+1))
                    v_size = interconnects[v_index].incomming.size();
                assert(v_size != -1 || h_size != -1);
                if(v_size == -1 || (h_size != -1 && h_size<v_size)){
                    //if(h_size > 0)
                    //    contentions++;
                    //interconnects[h_index].incomming.push(msg);
		    return h_index;
                }
                else{
                    //if(v_size > 0)
                    //    contentions++;
                    //interconnects[v_index].incomming.push(msg);
		    return v_index;
                }
            }
}

uint32_t Torus::num_proc(uint64_t addr){
    uint64_t num = get_directory_info(addr);
    uint32_t ans = 0;
    while(num != 0){
        if(num & 0x1)
            ans++;
        num = num >> 1;
    }
    return ans;
}

bool Torus::broadcast_needed(uint64_t addr){
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

void Torus::write_stats(const char *outfile){
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

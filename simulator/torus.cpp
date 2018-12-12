
#include <ostream>
#include <iostream>

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
        msg.receiver = nproc;
        send(msg);
        uint64_t current_state = get_directory_info(msg.addr);
        for(size_t i=0;i<nproc;i++){
            if(current_state & 0x1 && i!=msg.sender){
                msg.receiver = i;
                send(msg);
            }
        current_state = current_state>>1;
        }
    }
    else{
        if(msg.sender==MEM)
            msg.sender=nproc;
        send(msg);
    }
}

void Torus::send(CMsg msg){
    num_messages++;
    if(interconnects[msg.sender].incomming.size()>0)
        contentions++;
    interconnects[msg.sender].incomming.push(msg);
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
        msg.receiver = nproc;
        broadcast(msg);
    }
}

void Torus::event()
{
  for(size_t i=0;i<nproc+1;i++){
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
            CMsg msg = interconnects[i].incomming.front();
            assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
            int c_x = i%length;
            int c_y = i/length;
            int e_x = msg.receiver%length;
            int e_y = msg.receiver/length;
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
                assert(msg.receiver <= nproc);
                receivers[msg.receiver]->receive(msg);
            }
            else{
                size_t h_index = c_y*length + n_x;
                size_t v_index = n_y*length + c_x;
                int h_size = -1;
                int v_size = -1;
                if(h_index != i && h_index < (nproc+1))
                    h_size = interconnects[h_index].incomming.size();
                if(v_index != i && v_index < (nproc+1))
                    v_size = interconnects[v_index].incomming.size();
                assert(v_size != -1 || h_size != -1);
                if(v_size == -1 || (h_size != -1 && h_size<v_size)){
                    if(h_size > 0)
                        contentions++;
                    interconnects[h_index].incomming.push(msg);
                }
                else{
                    if(v_size > 0)
                        contentions++;
                    interconnects[v_index].incomming.push(msg);
                }
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


#include <ostream>
#include <iostream>

#include "crossbar.hpp"
#include "bus.hpp"
#include "mig_const.hpp"
#include "mig_memory.hpp"

////////////////////////////////////////////////////////////////////////////////
// Mesh
////////////////////////////////////////////////////////////////////////////////


void Mesh::broadcast(CMsg msg)
{
    assert(check_directory(msg.addr));
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

void Mesh::send(CMsg msg){
    int sent = msg.sender;
    if(numa && msg.sender == nproc + 1)
	sent = get_addr_proc(msg.addr);
    num_messages++;
    uint32_t recv = next(sent,msg);
    if(interconnects[sent*(nproc+1) + recv].incomming.size()>0)
        contentions++;
    interconnects[sent*(nproc+1) + recv].incomming.push(msg);
}

void Mesh::send_ack(CMsg msg)
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

void Mesh::event()
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
            CMsg msg = interconnects[i].incomming.front();
            assert(check_directory(msg.addr));
            interconnects[i].incomming.pop();
	    uint32_t s = i/(nproc+1);
	    uint32_t r = i%(nproc+1);
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


bool Mesh::check_directory(uint64_t addr){
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

uint32_t Mesh::next(uint32_t current, CMsg msg){
	    int c_x = current%length;
            int c_y = current/length;
	    size_t recv = msg.receiver;
	    if(numa && msg.receiver == nproc + 1)
		recv = get_addr_proc(msg.addr);
            int e_x = recv%length;
            int e_y = recv/length;
            int n_x = c_x;
            int n_y = c_y;
            if(c_x<e_x)
                n_x++;
            else if(c_x>e_x)
                n_x--;
            if(c_y<e_y)
                n_y++;
            else if(c_y>e_y)
                n_y = c_y - 1;
            if(n_x == e_x && n_y == e_y){
                //assert(msg.receiver <= nproc);
                //std::cout << "REACHED THE END" << std::endl;
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

uint32_t Mesh::num_proc(uint64_t addr){
    uint64_t num = get_directory_info(addr);
    uint32_t ans = 0;
    while(num != 0){
        if(num & 0x1)
            ans++;
        num = num >> 1;
    }
    return ans;
}

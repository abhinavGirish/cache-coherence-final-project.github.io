
#include "mesi_manager.hpp"

#include <utility>
#include <iomanip>


////////////////////////////////////////////////////////////////////////////////
// MESIManager
////////////////////////////////////////////////////////////////////////////////



MESIManager::MESIManager(Cache *cache, uint32_t proc, uint32_t nproc, RequestTable &request_table) : cache(cache), proc(proc), nproc(nproc), request_table(request_table) 
{
    pending_request.type = CMsgType::none;
}


std::ostream &operator<<(std::ostream &os, const MESIManager::PendingRequest &pend)
{
    os << "Pending request: { type: 0x" << pend.type << ", addr: 0x" << std::hex
        << pend.addr << std::dec << ", needed_acks: " << pend.needed_acks
        << ", tag: " << std::hex << pend.tag << std::dec;

    return os;
}


void MESIManager::event()
{
    handle_incomming();
    handle_pending_request();
}

void MESIManager::handle_incomming()
{
    // For messages we're not ready to ack yet; will put back for next time
    std::queue<CMsg> temp{};
    bool swap = false;
    while(!incomming.empty())
    {
        CMsg msg = incomming.front();
        if(msg.sender == proc)
        {
            incomming.pop();
            continue;
        }


        switch (msg.type)
        {
            case CMsgType::busRd:
            {
                if(cache->get_hit_pending() && msg.addr == cache->get_hit_addr())
                {
                    swap = true;
                    temp.push(msg);
                    break;
                }

                bool was_most_recent;
                bool found = cache->change_state(msg.addr, MESICoherenceState::STATE_SHARED, &was_most_recent);
                CMsg out = {
                    CMsgType::ack,
                    proc,
                    msg.sender,
                    msg.addr,
                    (found & MESIBusFlag::BUS_FLAG_SHARED) | (found & was_most_recent & MESIBusFlag::BUS_FLAG_TRANSMIT)
                };
                bus->send_ack(out);
                

                if(found && was_most_recent)
                {
                    CMsg data = {
                        CMsgType::data,
                        proc,
                        msg.sender,
                        msg.addr,
                        found ? MESIBusFlag::BUS_FLAG_SHARED : 0
                    };
                    bus->broadcast(data);
                }

                break;
            }
            case CMsgType::busRdX:
            {
                if(cache->get_hit_pending() && msg.addr == cache->get_hit_addr())
                {
                    swap = true;
                    temp.push(msg);
                    break;
                }

                bool was_most_recent;
                bool found = cache->change_state(msg.addr, MESICoherenceState::STATE_INVALID, 
                    &was_most_recent);
                cache->invalidate(msg.addr);
                CMsg out = {
                    CMsgType::ack,
                    proc,
                    msg.sender,
                    msg.addr,
                    (found & MESIBusFlag::BUS_FLAG_SHARED) | (found &was_most_recent & MESIBusFlag::BUS_FLAG_TRANSMIT)
                };
                bus->send_ack(out);

                if(found && was_most_recent)
                {
                    CMsg data = {
                        CMsgType::data,
                        proc,
                        msg.sender,
                        msg.addr,
                        found
                    };
                    bus->broadcast(data);
                }

                break;
            }
            case CMsgType::ack:
                if(pending_request.type != CMsgType::none
                    && msg.addr == pending_request.addr
                    && msg.receiver == proc
                    && msg.sender != MEM)
                {
                    pending_request.needed_acks--;
                }

                if(msg.flags & MESIBusFlag::BUS_FLAG_SHARED)
                {
                    pending_request.shared = true;
                }
                break;
            case CMsgType::data:
                if(pending_request.type != CMsgType::none
                    && msg.addr == pending_request.addr
                    && msg.receiver == proc)
                {
                    pending_request.got_data = true;
                }
                break;
            case CMsgType::none:
                // Should never happen
                assert(false);
                break;
        }

        incomming.pop();
    }

    if(swap)
    {
        incomming = temp;
    }
}

void MESIManager::handle_pending_request()
{
    // Check pending request
    /*
    if(logging && pending_request.type != CMsgType::none)
    {
        std::cout << "CM: proc: " << proc << ", pending_reissue " << pending_reissue << ", request type: " << pending_request.type << ", needed acks: " << pending_request.needed_acks << ", got data: " <<  pending_request.got_data << std::endl;
    }
    */
    // If we were been prempted last time, reissue
    if(pending_reissue && !request_table.exists(pending_request.addr))
    {
        // std::cout << "proc " << proc << " reissue" << std::endl;
        switch(pending_request.type)
        {
            case CMsgType::busRd:
                read(pending_request.addr);
                break;

                
            case CMsgType::busRdX:
                write(pending_request.addr);
                break;
            
            case CMsgType::data:
            case CMsgType::ack:
            case CMsgType::none:
                // Should never happen
                assert(false);
                break;
        }
    }
    // If the pending request is complete
    else if(!pending_reissue 
        && pending_request.type != CMsgType::none 
        && pending_request.needed_acks <= 0
        && pending_request.got_data)
    {
        // std::cout << "proc " << proc << " completex request" << std::endl;
        switch(pending_request.type)
        {
            case CMsgType::busRd:
            {
                size_t new_state = MESICoherenceState::STATE_EXCLUSIVE;
                if(pending_request.shared)
                {
                    new_state = MESICoherenceState::STATE_SHARED;
                }
                cache->complete_read(pending_request.addr, new_state);
                break;
            }
            case CMsgType::busRdX:
                cache->complete_write(pending_request.addr, MESICoherenceState::STATE_MODIFIED);
                break;

            case CMsgType::data:
            case CMsgType::ack:
            case CMsgType::none:
                // Should never happen
                assert(false);
                break;
  
        }

        request_table.del(pending_request.tag);
        pending_request.type = CMsgType::none;
        pending_reissue = false;
    }
    else
    {
        // std::cout << "proc " << proc << " did nothing" << std::endl;
    }



}

void MESIManager::read(uint64_t addr)
{
    /*
    if(proc == 10 && addr == 0x22e3300)
    {
        bus->log_on();
        cache->log_on();
        this->log_on();
    }
    */
    pending_request = {
        CMsgType::busRd,
        addr,
        nproc - 1,
        -1,
        false,
        false
    };

    if(request_table.exists(addr))
    {
        pending_reissue = true;
        return;
    }

    pending_request.tag = request_table.enter(addr, proc, pending_request.type);
    if(pending_request.tag < 0)
    {
        pending_reissue = true;
        return;
    }
    pending_reissue = false;

    CMsg msg = {
        CMsgType::busRd,
        proc,
        0, // doesn't apply for busRd
        addr,
        0
    };
    bus->broadcast(msg);
}


void MESIManager::write(uint64_t addr)
{
    if(proc == 10 && addr == 0x22e3300)
    {
        bus->log_on();
        cache->log_on();
        this->log_on();
    }
    Line *line;
    bool found = cache->find_line(addr, &line, false);

    pending_request = {
        CMsgType::busRdX,
        addr,
        nproc - 1,
        -1,
        found && line->most_recent,
        false
    };


    if(request_table.exists(addr))
    {
        pending_reissue = true;
        return;
    }

    pending_request.tag = request_table.enter(addr, proc, pending_request.type);
    if(pending_request.tag < 0)
    {
        pending_reissue = true;
        return;
    }
    pending_reissue = false;


    CMsg msg = {
        CMsgType::busRdX,
        proc,
        0, // doesn't matter for busRdX
        addr,
        0
    };
    bus->broadcast(msg);
}


bool MESIManager::can_read(size_t state)
{
    if(state == MESICoherenceState::STATE_INVALID)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool MESIManager::can_write(size_t state)
{
    if(state == MESICoherenceState::STATE_MODIFIED)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MESIManager::rw_upgrade(size_t state)
{
    if(state == MESICoherenceState::STATE_EXCLUSIVE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

size_t MESIManager::rw_upgrade_state(size_t state)
{
    state = 1; // to silence unused paramter warning 
    return MESICoherenceState::STATE_MODIFIED;
}





void MESIManager::print(std::ostream &os)
{
    os << "CM " << std::setw(2) << std::setfill('0') << this->proc << " "; 

    if(this->pending_reissue)
    {
        os << "waiting to reissue";
    }
    else if(this->pending_request.type != CMsgType::none)
    {
        os << "coherence request pending on 0x" << std::hex << this->pending_request.addr << std::dec;
    }
    else
    {
        os << "waiting";
    }
}

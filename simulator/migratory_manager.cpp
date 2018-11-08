
#include "migratory_manager.hpp"

#include <utility>


////////////////////////////////////////////////////////////////////////////////
// MigratoryManager
////////////////////////////////////////////////////////////////////////////////



MigratoryManager::MigratoryManager(Cache *cache, uint32_t proc, uint32_t nproc, RequestTable &request_table, bool do_mig) 
    : cache(cache), proc(proc), nproc(nproc), request_table(request_table), 
        do_mig(do_mig) 
{
    pending_request.type = CMsgType::none;
}


std::ostream &operator<<(std::ostream &os, const MigratoryManager::PendingRequest &pend)
{
    os << "Pending request: { type: 0x" << pend.type << ", addr: 0x" << std::hex
        << pend.addr << std::dec << ", needed_acks: " << pend.needed_acks
        << ", tag: " << std::hex << pend.tag << std::dec;

    return os;
}


void MigratoryManager::event()
{
    handle_incomming();
    handle_pending_request();
}

void MigratoryManager::recv_busRd(CMsg &msg)
{
    if(cache->get_hit_pending() && msg.addr == cache->get_hit_addr())
    {
        swap = true;
        incomming_temp.push(msg);
        return;
    }

    Line *line;
    bool found = cache->find_line(msg.addr, &line); 

    size_t flags = 0;
    if(found)
    {
        switch(line->state)
        {
            case MigCohState::INVALID:
                assert(false); // Should never happen
                break;

            case MigCohState::SHARED:
                line->state = MigCohState::SHARED;
                flags |= MigBusFlag::SHARED;
                break;

            case MigCohState::EXCLUSIVE:
            case MigCohState::DIRTY:
                if(do_mig)
                {
                    line->state = MigCohState::SHARED2;
                }
                else
                {
                    line->state = MigCohState::SHARED;
                }
                flags |= MigBusFlag::SHARED;
                break;

            case MigCohState::SHARED2:
                line->state = MigCohState::SHARED;
                flags |= MigBusFlag::SHARED;
                break;

            case MigCohState::MIGRATORY_CLEAN:
                if(do_mig)
                {
                    line->state = MigCohState::SHARED2;   
                }
                else
                {
                    line->state = MigCohState::SHARED;   
                }
                flags |= MigBusFlag::SHARED;
                break;

            case MigCohState::MIGRATORY_DIRTY:
                line->state = MigCohState::INVALID;
                flags |= MigBusFlag::MIGRATORY;
                cache->invalidate(msg.addr);
                // std::cout << "migratory send read" << std::endl;
                assert(do_mig);

                break;
        }

    }

    CMsg out = {
        CMsgType::ack,
        proc,
        msg.sender,
        msg.addr,
        flags | (found & line->most_recent & MigBusFlag::TRANSMIT)
    };
    bus->send_ack(out);
    

    if(found && line->most_recent)
    {
        CMsg data = {
            CMsgType::data,
            proc,
            msg.sender,
            msg.addr,
            1
        };
        bus->broadcast(data);
    }
    if(found)
    {
        line->most_recent = false;
    }
}

void MigratoryManager::recv_busRdX(CMsg &msg)
{
    if(cache->get_hit_pending() && msg.addr == cache->get_hit_addr())
    {
        swap = true;
        incomming_temp.push(msg);
        return;
    }

    // bool was_most_recent;
    // bool found = cache->change_state(msg.addr, MigCohState::INVALID, &was_most_recent);
    // cache->invalidate(msg.addr);


    Line *line;
    bool found = cache->find_line(msg.addr, &line); 

    size_t flags = 0;
    if(found)
    {
        switch(line->state)
        {
            case MigCohState::INVALID:
                assert(false); // Should never happen
                break;

            case MigCohState::SHARED:
                line->state = MigCohState::INVALID;
                cache->invalidate(msg.addr);
                break;

            case MigCohState::EXCLUSIVE:
            case MigCohState::DIRTY:
            case MigCohState::SHARED2:
                line->state = MigCohState::INVALID;
                cache->invalidate(msg.addr);
                if(do_mig)
                {
                    flags |= MigBusFlag::MIGRATORY;
                }
                break;

            case MigCohState::MIGRATORY_CLEAN:
                line->state = MigCohState::INVALID;
                cache->invalidate(msg.addr);
                break;

            case MigCohState::MIGRATORY_DIRTY:
                line->state = MigCohState::INVALID;
                flags |= MigBusFlag::MIGRATORY;
                assert(do_mig);
                cache->invalidate(msg.addr);
                break;
        }

    }




    CMsg out = {
        CMsgType::ack,
        proc,
        msg.sender,
        msg.addr,
        flags | (found & line->most_recent & MigBusFlag::TRANSMIT)
    };
    bus->send_ack(out);
    

    if(found && line->most_recent)
    {
        CMsg data = {
            CMsgType::data,
            proc,
            msg.sender,
            msg.addr,
            1
        };
        bus->broadcast(data);
    }
    if(found)
    {
        line->most_recent = false;
    }
}

void MigratoryManager::recv_ack(CMsg &msg)
{
    if(pending_request.type != CMsgType::none
        && msg.addr == pending_request.addr
        && msg.receiver == proc
        && msg.sender != MEM)
    {
        pending_request.needed_acks--;
    }

    if(msg.flags & MigBusFlag::SHARED)
    {
        pending_request.shared = true;
    }

    if(msg.flags & MigBusFlag::MIGRATORY)
    {
        pending_request.migratory = true;
    }
}


void MigratoryManager::recv_data(CMsg &msg)
{
    if(pending_request.type != CMsgType::none
        && msg.addr == pending_request.addr
        && msg.receiver == proc)
    {
        pending_request.got_data = true;
    }  
}

void MigratoryManager::handle_incomming()
{
    // For messages we're not ready to ack yet; will put back for next time
    swap = false;
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
                recv_busRd(msg);
                break;

            case CMsgType::busRdX:
                recv_busRdX(msg);
                break;

            case CMsgType::ack:
                recv_ack(msg);
                break;

            case CMsgType::data:
                recv_data(msg);
                break;

            case CMsgType::none:
                assert(false); // Should never happen
                break;
        }

        incomming.pop();
    }

    if(swap)
    {
        std::swap(incomming, incomming_temp);
    }
}

void MigratoryManager::handle_pending_request()
{
    // Check pending request
    // std::cout << "proc " << proc << " " << pending_request.needed_acks << " " << pending_request.got_data << std::endl;

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
                size_t new_state;
                if(pending_request.migratory && do_mig)
                {
                    // std::cout << "read migratory" << std::endl;
                    new_state = MigCohState::MIGRATORY_CLEAN;
                }
                else if(pending_request.shared)
                {
                    new_state = MigCohState::SHARED;
                }
                else
                {
                    new_state = MigCohState::EXCLUSIVE;
                }

                cache->complete_read(pending_request.addr, new_state);
                break;
            }
            case CMsgType::busRdX:
            {
                Line *line = 0;
                bool found = cache->find_line(pending_request.addr, &line, false);

                size_t new_state;
                if(found)
                {
                    if(line->state == MigCohState::EXCLUSIVE
                        || line->state == MigCohState::SHARED2)
                    {
                        new_state = MigCohState::DIRTY;
                    }
                    else if(line->state == MigCohState::SHARED)
                    {
                        if(pending_request.migratory && do_mig)
                        {
                            new_state = MigCohState::MIGRATORY_DIRTY;
                        }
                        else
                        {
                            new_state = MigCohState::DIRTY;
                        }
                    }
                    else if(do_mig)// was MIGRATORY_CLEAN
                    {
                        new_state = MigCohState::MIGRATORY_DIRTY;
                    }
                    else 
                    {
                        assert(false);
                    }
                }
                else // was INVALID
                {
                    if(pending_request.migratory && do_mig)
                    {
                        new_state = MigCohState::MIGRATORY_DIRTY;
                    }
                    else
                    {
                        new_state = MigCohState::DIRTY;
                    }
                }
                
                cache->complete_write(pending_request.addr, new_state);
                break;
            }
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

void MigratoryManager::read(uint64_t addr)
{

    pending_request = {
        CMsgType::busRd,
        addr,
        nproc - 1,
        -1,
        false,
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


void MigratoryManager::write(uint64_t addr)
{

    pending_request = {
        CMsgType::busRdX,
        addr,
        nproc - 1,
        -1,
        false,
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
        CMsgType::busRdX,
        proc,
        0, // doesn't matter for busRdX
        addr,
        0
    };
    bus->broadcast(msg);
}


bool MigratoryManager::can_read(size_t state)
{
    if(state == MigCohState::INVALID)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool MigratoryManager::can_write(size_t state)
{
    if(state == MigCohState::DIRTY || state == MigCohState::MIGRATORY_DIRTY)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MigratoryManager::rw_upgrade(size_t state)
{
    if(state == MigCohState::EXCLUSIVE || state == MigCohState::MIGRATORY_CLEAN)
    {
        return true;
    }
    else
    {
        return false;
    }
}

size_t MigratoryManager::rw_upgrade_state(size_t state)
{
    if(state == MigCohState::EXCLUSIVE)
    {
        return MigCohState::DIRTY;
    }
    else if(state == MigCohState::MIGRATORY_CLEAN)
    {
        return MigCohState::MIGRATORY_DIRTY;
    }

    assert(false); // State cannot be upgraded; should never happen
}





void MigratoryManager::print(std::ostream &os)
{
    os << "CM " << this->proc << " "; 

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

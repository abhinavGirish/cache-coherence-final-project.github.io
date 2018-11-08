
#include "mesi_memory.hpp"

#include <iostream>

void MESIMemory::event()
{
    // Check pending memops
    for(auto it=table.begin(); it != table.end(); /*empty*/ )
    {
        if(it->second.count.tick())
        {
            // Memop is done, send the data, delete from table
            CMsg msg
            {
                CMsgType::data,
                MEM,
                it->second.requester,
                it->first,
                MESIBusFlag::BUS_FLAG_SHARED,
            };
            bus->broadcast(msg);

            table.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    while(!incomming.empty())
    {
        CMsg msg = incomming.front();

        switch (msg.type)
        {
            case CMsgType::busRd:
            case CMsgType::busRdX:
            {
                // Start the read, just in case
                PendingMemOp op = 
                {
                    Countdown(latency),
                    msg.sender
                };
                table[msg.addr] = op;
                break;
            }
            case CMsgType::ack:
                // if someone else supplied the data
                if(table.count(msg.addr) > 0 && msg.flags & MESIBusFlag::BUS_FLAG_TRANSMIT)
                {
                    table.erase(msg.addr);
                }
                break;

            case CMsgType::data:
                break; // Not relevant
            case CMsgType::none:
                // Should never happen
                assert(false);
                break;
        }

        incomming.pop();
    }
}
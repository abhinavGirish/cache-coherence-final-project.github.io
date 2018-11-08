
#include "request_table.hpp"
#include <algorithm>

RequestTable::RequestTable(int size)
{
    Entry empty = {0, 0, CMsgType::none};
    table.resize(size, empty);
}

bool RequestTable::exists(uint64_t addr)
{
    return std::any_of(table.begin(), table.end(), [addr](const Entry &e)
        { return e.addr == addr && e.type != CMsgType::none; });
}

uint32_t RequestTable::enter(uint64_t addr, uint32_t requester, CMsgType type)
{
    for(uint32_t i=0; i<table.size(); i++)
    {   
        Entry &e = table.at(i);
        if(e.type != CMsgType::none)
        {
            if(e.addr == addr)
            {
                return -1;
            }
            continue;
        }
        e.addr = addr;
        e.requester = requester;
        e.type = type;

        return i;
    }

    return -1;
}

void RequestTable::del(uint32_t tag)
{
    table.at(tag).type = CMsgType::none;
}


std::ostream &operator<<(std::ostream &os, const RequestTable::Entry &e)
{
    os << "Entry { addr: 0x" << std::hex << e.addr << std::dec
        << ", requester: " << e.requester << ", type: " << e.type << " }";

    return os;
}

std::ostream &operator<<(std::ostream &os, const RequestTable &tble)
{
    os << "Request Table" << std::endl;
    os << "[" << std::endl;
    for(auto& entry : tble.table)
    {
        os << "\t" << entry << std::endl;
    }
    os << "]" << std::endl;

    return os;
}


#ifndef REQUEST_TABLE_HPP
#define REQUEST_TABLE_HPP

#include <iostream>


#include "bus.hpp"

class MESIConsistencyChecker;
class MIGConsistencyChecker;

class RequestTable
{
    friend MESIConsistencyChecker;
    friend MIGConsistencyChecker;

private:
    struct Entry 
    {
        uint64_t addr;
        uint32_t requester;
        CMsgType type;
    };
    std::vector<Entry> table;
    

public:
    RequestTable(int size);
    bool exists(uint64_t addr);
    uint32_t enter(uint64_t addr, uint32_t requester, CMsgType type);
    void del(uint32_t tag);


    friend std::ostream &operator<<(std::ostream &os, const Entry &e);
    friend std::ostream &operator<<(std::ostream &os, const RequestTable &tble);
};


#endif // REQUEST_TABLE_HPP
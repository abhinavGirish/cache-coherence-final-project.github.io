
#include <set>
#include <string>
#include <iostream>
#include <ios>
#include <utility>

#include "mesi_consistency_checker.hpp"



void MESIConsistencyChecker::check_states()
{
    std::vector<string> errors;
    for(uint32_t i=0; i<num_sets; i++)
    {
        std::map<uint64_t, std::vector<std::pair<uint32_t, uint32_t>>> modified_modified;
        std::map<uint64_t, std::vector<std::pair<uint32_t, uint32_t>>> modified_other;

        for(uint32_t j=0; j<caches.size(); j++)
        {
            auto cache = caches.at(j);

            Cache::Set &set = cache->data[i];
            for(auto &line : set)
            {
                uint64_t addr = cache->base_addr(i, line.tag);
                if(line.state == MESICoherenceState::STATE_MODIFIED)
                {
                    if(modified_modified.count(addr) > 0)
                    {
                        modified_modified[addr].push_back(std::make_pair(j, i));
                    }
                    else
                    {
                        std::vector<std::pair<uint32_t, uint32_t>> vec{std::make_pair(j, i)};
                        modified_modified[addr] = vec;
                        modified_other[addr] = vec;
                    }
                }
                else if(line.state == MESICoherenceState::STATE_SHARED)
                {
                    if(modified_other.count(addr) > 0)
                    {
                        modified_other[addr].push_back(std::make_pair(j, i));
                    }
                }
            }
        }

        for(auto &elm : modified_modified)
        {
            if(elm.second.size() > 1)
            {
                std::cerr << std::endl << "Error: addr 0x" << std::hex << elm.first << std::dec << " is modified in:" << std::endl;
                std::cerr << "\tcache\tset" << std::endl;
                for(auto &entry : elm.second)
                {
                    std::cerr << "\t" << entry.first << "\t" << entry.second << std::endl;
                }
                std::cout << std::endl;
            }
        }

        for(auto &elm : modified_other)
        {
            if(elm.second.size() > 1)
            {
                 std::cerr << std::endl << "Error: addr 0x" << std::hex << elm.first << std::dec << " is modified/shared in:" << std::endl;
                std::cerr << "\tcache\tset" << std::endl;
                for(auto &entry : elm.second)
                {
                    std::cerr << "\t" << entry.first << "\t" << entry.second << std::endl;
                }
                std::cout << std::endl;
            }
        }
    }
}


void MESIConsistencyChecker::check_requests()
{
    for(size_t i=0; i<table.table.size(); i++)
    {
        auto &addr = table.table[i].addr;
        if(table.table[i].type == CMsgType::none)
        {
            continue;
        }

        std::vector<RequestTable::Entry> bad{};
        for(size_t j=i+1; j<table.table.size(); j++)
        {
            if(table.table[j].type != CMsgType::none
                && table.table[j].addr == addr)
            {
                bad.push_back(table.table[j]);
            }
        }

        if(bad.size() > 0)
        {
            std::cout << "addr 0x" << std::hex << addr << std::dec << " has multiple pending requests" << std::endl;

            for(auto &e : bad)
            {
                std::cout << "\t" << e << std::endl;
            }
        }
    }
}

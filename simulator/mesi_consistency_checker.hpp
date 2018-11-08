

#ifndef MESI_CONSISTENCY_CHECKER_HPP
#define MESI_CONSISTENCY_CHECKER_HPP

#include <vector>
#include <cstdint>

#include "cache.hpp"
#include "bus.hpp"
#include "mesi_const.hpp"
#include "consistency_checker_interface.hpp"
#include "request_table.hpp"


class MESIConsistencyChecker : public ConsistencyCheckerInterface
{
private:
    std::vector<Cache*> caches;
    CacheConfig config;
    uint32_t num_sets;
    RequestTable &table;

public:
    MESIConsistencyChecker(std::vector<Cache*> caches, RequestTable &table, CacheConfig config) 
        : caches(caches), config(config), table(table)
    {
        num_sets = pow(2, this->config.set_bits);
    }


    void check_states();
    void check_requests();
    void check() { check_states(); check_requests(); }
};



#endif // MESI_CONSISTENCY_CHECKER_HPP

#ifndef CACHE_HPP
#define CACHE_HPP



#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <list>
#include <queue>
#include <memory>

#include "event.hpp"
#include "task_manager.hpp"
#include "countdown.hpp"
#include "bus.hpp"
#include "coherence_manager_interface.hpp"
#include "coherence_manager_misc.hpp"





struct Line
{
   uint32_t tag;
   size_t state;
   bool most_recent;
   bool valid;
   // Don't care about actual data
};


std::ostream &operator<<(std::ostream& os, const Line &line);


// Only valid for a particular cache!!!
struct ParsedAddr
{
    uint32_t set;
    uint64_t tag;
    uint64_t addr;
};


struct CacheStats
{
    uint64_t write_hits;
    uint64_t write_misses;

    uint64_t read_hits;
    uint64_t read_misses;

    uint64_t evictions;

    // friend CacheStats operator+(const CacheStats &rhs);
    CacheStats &operator+=(const CacheStats &rhs);
};
CacheStats operator+(CacheStats lhs, const CacheStats &rhs);
std::ostream& operator<<(std::ostream& os, const CacheStats &stats);


struct CacheConfig
{
    uint32_t nproc;
    uint32_t block_bits;
    uint32_t set_bits;
    uint32_t num_lines;
    uint32_t read_hit_latency;
    uint32_t write_hit_latency;
};


class MESIConsistencyChecker;
class MIGConsistencyChecker;

class Cache : public Event
{
    friend MESIConsistencyChecker;
    friend MIGConsistencyChecker;
    friend std::ostream &operator<<(std::ostream &os, const Cache &c);
private:
    TaskManager &manager;
    uint32_t proc;
    CacheConfig config;

    using Set = std::list<Line>;
    std::vector<Set> data;

    CoherenceManagerInterface *coherence_manager;

    uint32_t num_sets;
    uint32_t block_mask;
    CacheStats stats = {};

    bool hit_pending = false;
    Countdown read_hit_countdown{};
    Countdown write_hit_countdown{};
    uint64_t hit_addr;

    bool done = false;

    bool logging = false;

    bool coherence_request_pending = false;

    ParsedAddr parse_addr(std::uint64_t addr);
    uint64_t base_addr(uint32_t set, uint64_t tag);

    void allocate(ParsedAddr parsed, size_t new_state);

    void issue_new();

public:
    Cache(TaskManager &manager, uint32_t proc, CacheConfig config);

    void event();

    void set_coherence_manager(CoherenceManagerInterface *m) 
        { this->coherence_manager = m; }
    const CacheConfig &get_cache_config() { return config; }
    bool is_done() { return done; }
    uint32_t get_proc() { return proc; }
    CacheStats get_stats() { return stats; }
    bool get_hit_pending() { return hit_pending; }
    bool get_hit_addr() { return hit_addr; }

    void read(uint64_t addr);
    void complete_read(uint64_t addr, size_t new_state);

    void write(uint64_t addr);
    void complete_write(uint64_t addr, size_t new_state);

    bool find_line(uint64_t addr, Line **line);
    bool find_line(uint64_t addr, Line **line, bool move_to_back);
    bool find_line(ParsedAddr parsed, Line **line);
    bool find_line(ParsedAddr parsed, Line **line, bool move_to_back);

    bool change_state(uint64_t addr, size_t state, bool *was_most_recent);
    bool change_state(uint64_t addr, size_t state);

    void invalidate(uint64_t addr);

    void log_on() { logging = true; }
    void log_off() { logging = false; }

    friend std::ostream &operator<<(std::ostream &os, const Set &s);
    friend std::ostream &operator<<(std::ostream &os, const Cache &c);
};




#endif // CACHE_HPP
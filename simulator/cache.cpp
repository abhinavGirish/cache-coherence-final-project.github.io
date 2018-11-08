

#include "cache.hpp"

#include <iomanip>


std::ostream &operator<<(std::ostream& os, const Line &line)
{
    os << "Line {tag: 0x" << std::hex << line.tag << std::dec
        << ", state: " << line.state << ", most recent: " << line.most_recent 
        << " }";

    return os;
}

////////////////////////////////////////////////////////////////////////////////
// Cache Stats
////////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& os, const CacheStats &stats)
{
    double write_miss_rate = (((float)stats.write_misses) / 
        (((float)stats.write_misses) + ((float)stats.write_hits))) * 100;

    double read_miss_rate = (((float)stats.read_misses) / 
        (((float)stats.read_misses) + ((float)stats.read_hits))) * 100;


    os << "write hits\t" << stats.write_hits << std::endl;
    os << "write misses\t" << stats.write_misses << std::endl;
    os << "write miss rate\t" << setprecision(2) <<  write_miss_rate << "%"<< std::endl << std::endl;
    os << "read hits\t" << stats.read_hits << std::endl;
    os << "read misses\t" << stats.read_misses << std::endl;
    os << "read miss rate\t" << setprecision(2) << read_miss_rate << "%"<< std::endl << std::endl;

    os << "evictions\t" << stats.evictions << std::endl << std::endl;;

    return os;
}



CacheStats &CacheStats::operator+=(const CacheStats &rhs)
{
    this->write_hits += rhs.write_hits;
    this->write_misses += rhs.write_misses;

    this->read_hits += rhs.read_hits;
    this->read_misses += rhs.read_misses;

    this->evictions += rhs.evictions;

    return *this;
}

CacheStats operator+(CacheStats lhs, const CacheStats &rhs)
{
    lhs += rhs;
    return lhs;
}


////////////////////////////////////////////////////////////////////////////////
// Cache
////////////////////////////////////////////////////////////////////////////////

Cache::Cache(TaskManager &manager, uint32_t proc, CacheConfig config) 
    : manager(manager), proc(proc), config(config)
{
    num_sets = (int)pow(2, config.set_bits);
    block_mask = ~(int)(pow(2, config.block_bits) - 1);

    // Create empty cache
    Line empty_line = Line();
    empty_line.valid = false;

    Set empty_set(config.num_lines, empty_line);
    data.resize(num_sets, empty_set);

    // pending_request.type = CMsgType::none;
    // pending_reissue = false;
}


bool Cache::find_line(ParsedAddr parsed, Line **line, bool move_to_back)
{
    Set &set = data.at(parsed.set);
    for(Set::iterator it = set.begin(); it != set.end(); it++)
    {
        if(it->tag == parsed.tag && it->valid)
        {
            *line = &*it;
            if(move_to_back)
            {
                set.splice(set.end(), set, it);
            }
            return true;
        }
    }

    return false;
}


bool Cache::find_line(ParsedAddr parsed, Line **line)
{
    return this->find_line(parsed, line, true);
}

bool Cache::find_line(uint64_t addr, Line **line)
{
    return this->find_line(parse_addr(addr), line, true);
}

bool Cache::find_line(uint64_t addr, Line **line, bool move_to_back)
{
    return this->find_line(parse_addr(addr), line, move_to_back);
}

void Cache::allocate(ParsedAddr parsed, size_t new_state)
{
    Set &set = data.at(parsed.set);

    for(Set::iterator it = set.begin(); it != set.end(); it++)
    {
        if(it->valid == false)
        {
            it->most_recent = true;
            it->tag = parsed.tag;
            it->state = new_state;
            it->valid = true;
            set.splice(set.end(), set, it);
            return;
        }
    }

    stats.evictions++;
    Set::iterator it = set.begin();
    it->most_recent = true;
    it->tag = parsed.tag;
    it->state = new_state;
    it->valid = true;
    set.splice(set.end(), set, it);
}




void Cache::issue_new()
{
    contech::Action action;
    if(manager.get_next_memop(proc, action))
    {
        contech::MemoryAction *mem = (contech::MemoryAction*)(&action);
        if(action.getType() == contech::action_type_mem_read)
        {
            read(mem->addr & block_mask);
        }
        else if(action.getType() == contech::action_type_mem_write)
        { 
            write(mem->addr & block_mask);
        }
    }
    else
    {
        if(manager.is_done())
        {
            done = true;
            return;
        }
    }
}

void Cache::event()
{
    if(logging && proc == 0)
    {
        std::cout << "proc 0 " << "read hit remaining: " << read_hit_countdown.get_remaining() << ", write hit remaining: " << write_hit_countdown.get_remaining() << std::endl;
    }
    if(!read_hit_countdown.is_done())
    {
        if(read_hit_countdown.tick())
        {
            stats.read_hits++;
            hit_pending = false;
        }
    }
    else if(!write_hit_countdown.is_done())
    {
        if(write_hit_countdown.tick())
        {
            stats.write_hits++;
            hit_pending = false;
        }

    }


    // Check for new requests
    if( // !pending_reissue
        // && pending_request.type == CMsgType::none
        !coherence_request_pending
        && read_hit_countdown.is_done()
        && write_hit_countdown.is_done())
    {
        issue_new();
    }

}


void Cache::read(uint64_t addr)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line);
    if(found && coherence_manager->can_read(line->state))
    {
        hit_pending = true;
        read_hit_countdown.reset(config.read_hit_latency);
        hit_addr = addr;
    }
    else
    {
        stats.read_misses++;
        coherence_request_pending = true;
        coherence_manager->read(addr);
    }
    
}


void Cache::complete_read(uint64_t addr, size_t new_state)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line);
    if(found)
    {
        line->most_recent = true;
        line->state = new_state;
    }
    else
    {
        this->allocate(parsed, new_state);
    }

    coherence_request_pending = false;
}


void Cache::write(uint64_t addr)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line);
    if(found)
    {

        if(coherence_manager->can_write(line->state))
        {
            hit_pending = true;
            write_hit_countdown.reset(config.write_hit_latency);
            hit_addr = addr;
            return;
        }
        else if(coherence_manager->rw_upgrade(line->state))
        {
            line->state = coherence_manager->rw_upgrade_state(line->state);
            hit_pending = true;
            write_hit_countdown.reset(config.write_hit_latency);
            hit_addr = addr;
            return;
        }
    }

    stats.write_misses++;
    coherence_request_pending = true;
    this->coherence_manager->write(addr);

}


void Cache::complete_write(uint64_t addr, size_t new_state)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line);
    if(found)
    {
        line->most_recent = true;
        line->state = new_state;
    }
    else
    {
        this->allocate(parsed, new_state);
    }

    coherence_request_pending = false;
}



bool Cache::change_state(uint64_t addr, size_t state, bool *was_most_recent)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line, false);
    if(found)
    {

        line->state = state;

        if(was_most_recent != nullptr)
        {
            *was_most_recent = line->most_recent;
        }
        line->most_recent = false;
    }
    else
    {
        *was_most_recent = false;
    }
    return found;
}


bool Cache::change_state(uint64_t addr, size_t state)
{
    return change_state(addr, state, nullptr);
}


void Cache::invalidate(uint64_t addr)
{
    ParsedAddr parsed = parse_addr(addr);
    Line *line;
    bool found = find_line(parsed, &line, false);
    if(found)
    {
        line->valid = false;
    }
}


// Result only valid for a particular cache!!!
ParsedAddr Cache::parse_addr(uint64_t addr)
{
    ParsedAddr parsed;
    parsed.addr = addr;
    addr >>= config.block_bits;
    uint64_t set_mask = num_sets - 1;
    parsed.set = addr & set_mask;
    parsed.tag = addr >> config.set_bits;

    return parsed;
}


uint64_t Cache::base_addr(uint32_t set, uint64_t tag)
{
    uint64_t addr = tag;
    addr <<= config.set_bits;
    addr |= set;
    addr <<= config.block_bits;

    return addr;
}


std::ostream &operator<<(std::ostream &os, const Cache::Set &s)
{
    os << "Set [" << std::endl;
    for(auto &l : s)
    {
        os << "\t" << l << std::endl;
    }
    os << "]" << std::endl;

    return os;
}


std::ostream &operator<<(std::ostream &os, const Cache &c)
{
    os << "Cache " << c.proc << " ";

    if(c.done)
    {
        os << "done";
    }
    else if(c.hit_pending)
    {
        os << "hit pending";
    }
    else if(c.coherence_request_pending)
    {
        os << "coherence request pending";
    }
    else 
    {
        os << "unknown state";
    }

    return os;
}
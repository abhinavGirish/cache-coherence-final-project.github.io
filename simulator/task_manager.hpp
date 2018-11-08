
#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP


#include "../contech/common/taskLib/TaskGraph.hpp"
#include <queue>
#include <unordered_set>

struct comp_task_gt
{
    bool operator()(contech::Task *lhs, contech::Task *rhs)
    {
        return lhs->getStartTime() > rhs->getStartTime();
    }
};


class TaskManager
{
private:
    contech::TaskGraph &graph;
    std::priority_queue<contech::Task*, std::vector<contech::Task*>, comp_task_gt> queue;
    std::vector<contech::Task*> ready;
    std::vector<uint64_t> ready_indices;
    std::unordered_set<contech::TaskId> retired;

    const int QUEUE_SIZE = 32;
    bool no_more_tasks = false;
    bool done = false;
    uint64_t task_limit = -1;
    uint64_t task_counter = 0;

    bool roi;
    contech::TaskId roi_start;
    contech::TaskId roi_end;

    bool logging = false;

    contech::Task *get_next_basic_block();
    void refill_queue();
    bool all_done();


public:
    TaskManager(contech::TaskGraph &graph);
    TaskManager(contech::TaskGraph &graph, bool roi);
    TaskManager(contech::TaskGraph &graph, bool roi, std::uint64_t task_limit);

    void initialize();
    void goto_roi();
    bool get_next_action(contech::ContextId context, contech::Action &ret);
    bool get_next_memop(contech::ContextId context, contech::Action &ret);
    bool is_done() { return done; };
    contech::TaskGraph &get_task_graph() { return graph; };

    uint64_t get_count() { return task_counter; }
    bool preds_met(contech::Task *top);

    contech::TaskId get_roi_start() { return roi_start; }
    contech::TaskId get_roi_end() { return roi_end; }

    void log_on() { logging = true; }
    void log_off() { logging = false; }
};






#endif // TASK_MANAGER_HPP


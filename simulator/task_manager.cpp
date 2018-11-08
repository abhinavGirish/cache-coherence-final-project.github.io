#include <algorithm>

#include "task_manager.hpp"



TaskManager::TaskManager(contech::TaskGraph &graph) : graph(graph) 
{
    initialize();
}


TaskManager::TaskManager(contech::TaskGraph &graph, bool roi)
    : graph(graph), roi(roi)
{
    if(roi)
    {
        goto_roi();
    }   
    initialize();
}
TaskManager::TaskManager(contech::TaskGraph &graph, bool roi, 
    std::uint64_t task_limit)
    : graph(graph), task_limit(task_limit), roi(roi)
{
    if(roi)
    {
        goto_roi();
    }   
    initialize();
}

void TaskManager::goto_roi()
{
    roi_start = graph.getROIStart();
    roi_end = graph.getROIEnd();
    contech::Task *task;
    while((task = graph.getNextTask()) && task->getTaskId() != roi_start)
    {
        task_counter++;
        retired.insert(task->getTaskId());
        delete task;
    } 

}

void TaskManager::initialize()
{
    ready.resize(graph.getNumberOfContexts(), nullptr);
    ready_indices.resize(graph.getNumberOfContexts(), 0);
}

contech::Task *TaskManager::get_next_basic_block()
{
    contech::Task *task;
    while(1)
    {
        task = graph.getNextTask();
        if(task != nullptr)
        {
            if(roi && task->getTaskId() == roi_end)
            {
                no_more_tasks = true;
                delete task;
                return nullptr;
            }

            if(task->getType() == contech::task_type_basic_blocks)
            {
                return task;
            }
            else
            {
                task_counter++;
                retired.insert(task->getTaskId());
                delete task;
            }
        }
        else
        {
            no_more_tasks = true;
            return nullptr;
        }
    }
}


void TaskManager::refill_queue()
{

    // If its empty, add in the next QUEUE_SIZE Tasks
    for(int i=0; i<QUEUE_SIZE; i++)
    {
        contech::Task *task = get_next_basic_block();
        if(task == nullptr)
        {
            return;
        }
        queue.push(task);
    }
}

bool TaskManager::get_next_action(contech::ContextId context, contech::Action &ret)
{   

    if(done)
    {
        return false;
    }
    uint32_t context_id = uint32_t(context);
    contech::Task *task;
    if((task = ready.at(context_id)) != nullptr)
    {
        uint64_t next = ready_indices.at(context_id)++;
        
        // Check if the current Task is complete; if not, we can grab the next  
        //  Action in this task
        if(next < task->getActions().size())
        {

            if(logging)
            {
                std::cout << "proc " << context_id << " got next action" << std::endl;
            }
            ret = task->getActions()[next];
            return true;
        }


        // If we've run out of Actions in this Task, put the TaskID in the
        //  retired set and check if there's another task
        retired.insert(task->getTaskId());
        delete task;
        ready[context_id] = nullptr;
        ready_indices[context_id] = 0;

        task_counter++;

        if(task_limit > 0 && task_counter >= task_limit)
        {
            task_counter = graph.getNumberOfTasks();
            done = true;
            return false;
        }
    
        
    }

    // We have to check to see if there's another Task for this context 
    //  in the graph with all dependencies satisfied
    if(queue.empty())
    {
        if(no_more_tasks)
        {
            if(all_done())
            {
                task_counter = graph.getNumberOfTasks();
                done = true;
            }

            if(logging)
            {
                std::cout << "all done" << std::endl;
            }
            return false;
        }
        else 
        {
            refill_queue();
            if(queue.empty())
            {

                if(logging)
                {
                    std::cout << "queue not refilled" << std::endl;
                }
                return false;
            }
        }
    }
    contech::Task *top = queue.top();
    if(top->getContextId() == context_id && preds_met(top))
    {
        // If the next available Task belongs to our context, and all its
        //  dependencies are met 
        queue.pop();
        ready[context_id] = top;
        ready_indices[context_id] = 0;


        if(logging)
        {
            std::cout << "TM: proc " << context_id << " got next task" << std::endl;
        }
        return get_next_action(context, ret);
    }
    else
    {   
        if(logging)
        {
            std::cout << "TM: proc " << context_id << " not ready" << std::endl;
            if(all_done())
            {
                std::cout << "TM: all done" << std::endl;
            }
        }
        // There are dependencies not yet met
        return false;
    }
}

bool TaskManager::all_done()
{
    return std::all_of(ready.begin(), ready.end(), 
        [](contech::Task *task){return task == 0; });
}

bool TaskManager::get_next_memop(contech::ContextId context, contech::Action &ret)
{
    bool found;
    while((found = get_next_action(context, ret))
        && !ret.isMemOp()) {  } // Skip non-memops

    return found;
}


bool TaskManager::preds_met(contech::Task *top)
{
    vector<contech::TaskId> &preds = top->getPredecessorTasks();
    return std::all_of(preds.begin(), preds.end(),
            [this](contech::TaskId id){return this->retired.count(id)>0;});
}
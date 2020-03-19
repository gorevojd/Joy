#ifndef JOY_ENGINE_UTIL_H
#define JOY_ENGINE_UTIL_H

#include "joy_platform.h"
#include "joy_memory.h"

struct task_data{
    task_data* Next;
    task_data* Prev;
    
    mem_block Block;
};

struct task_data_pool{
    int FreeTasksCount;
    int TotalTasksCount;
    
    platform_mutex Mutex;
    
    mi MemUsed;
    
    task_data UseSentinel;
    task_data FreeSentinel;
};

// NOTE(Dima): Functions definitions
task_data* BeginTaskData(task_data_pool* Pool);
void EndTaskData(task_data_pool* Pool, task_data* Task);
void InitTaskDataPool(task_data_pool* Pool,
                      mem_region* Region,
                      int TasksDatasCount,
                      mi OneSize);

#endif
#include "joy_engine.h"

// NOTE(Dima): task_data functions
task_data* BeginTaskData(task_data_pool* Pool)
{
    task_data* Result = 0;
    
    Platform.LockMutex(&Pool->Mutex);
    if(Pool->FreeTasksCount)
    {
        Result = Pool->FreeSentinel.Next;
        
        DLIST_REMOVE_ENTRY(Result, Next, Prev);
        DLIST_INSERT_BEFORE_SENTINEL(Result, Pool->UseSentinel, Next, Prev);
        
        --Pool->FreeTasksCount;
    }
    Platform.UnlockMutex(&Pool->Mutex);
    
    return(Result);
}

void EndTaskData(task_data_pool* Pool, task_data* Task)
{
    Platform.LockMutex(&Pool->Mutex);
    
    DLIST_REMOVE_ENTRY(Task, Next, Prev);
    DLIST_INSERT_BEFORE_SENTINEL(Task, Pool->FreeSentinel, Next, Prev);
    ++Pool->FreeTasksCount;
    
    Platform.UnlockMutex(&Pool->Mutex);
}

void InitTaskDataPool(task_data_pool* Pool,
                      mem_region* Region,
                      int TasksDatasCount,
                      mi OneSize)
{
    Platform.InitMutex(&Pool->Mutex);
    
    Pool->UseSentinel = {};
    Pool->FreeSentinel = {};
    
    DLIST_REFLECT_PTRS(Pool->UseSentinel, Next, Prev);
    DLIST_REFLECT_PTRS(Pool->FreeSentinel, Next, Prev);
    
    Pool->FreeTasksCount = TasksDatasCount;
    Pool->TotalTasksCount = TasksDatasCount;
    
    task_data* PoolArray = PushArray(Region, task_data, TasksDatasCount);
    
    for(int NewIndex = 0;
        NewIndex < TasksDatasCount;
        NewIndex++)
    {
        task_data* Task = PoolArray + NewIndex;
        
        Task->Block = PushSplit(Region, OneSize);
        
        DLIST_INSERT_BEFORE_SENTINEL(Task, 
                                     Pool->FreeSentinel, 
                                     Next, Prev);
    }
}
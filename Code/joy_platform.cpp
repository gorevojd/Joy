#include "joy_platform.h"

#define PLATFORM_USE_STD_MUTEX 1

PLATFORM_READ_FILE(PlatformReadFile) {
    Platform_Read_File_Result res = {};
    
	FILE* fp = fopen(filePath, "rb");
    
	if (fp) {
		fseek(fp, 0, 2);
		u64 fileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		res.dataSize = fileSize;
		res.data = (u8*)calloc(fileSize + 1, 1);
        
		fread(res.data, 1, fileSize, fp);
        
		fclose(fp);
	}
    
	return(res);
}

PLATFORM_WRITE_FILE(PlatformWriteFile) {
	FILE* file = (FILE*)fopen(filePath, "wb");
    
    b32 res = 0;
    
	if (file) {
		size_t elementsWritten = fwrite(data, 1, size, file);
        
        res = (elementsWritten == size);
        
		fclose(file);
	}
    
    return(res);
}

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory) {
	if (fileReadResult->data) {
		free(fileReadResult->data);
	}
	fileReadResult->data = 0;
}

PLATFORM_ADD_ENTRY(PlatformAddEntry){
#if PLATFORM_USE_STD_MUTEX
    queue->addMutexSTD.lock();
#else
    BeginTicketMutex(&queue->addMutex);
#endif
    
    uint32_t oldAddIndex = queue->addIndex.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    
    uint32_t newAddIndex = (oldAddIndex + 1) % queue->jobsCount;
    // NOTE(Dima): We should not overlap
    Assert(newAddIndex != queue->doIndex.load(std::memory_order_acquire));
    
    PlatformJob* job = &queue->jobs[oldAddIndex];
    job->callback = callback;
    job->data = data;
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->addIndex.store(newAddIndex, std::memory_order_relaxed);
    queue->started.fetch_add(1);
    
    queue->conditionVariable.notify_all();
#if PLATFORM_USE_STD_MUTEX
    queue->addMutexSTD.unlock();
#else
    EndTicketMutex(&queue->AddMutex);
#endif
}

InternalFunction b32 PlatformDoWorkerWork(Platform_Job_Queue* queue){
    b32 res = 0;
    
    std::uint32_t d = queue->doIndex.load();
    
    if(d != queue->addIndex.load(std::memory_order_acquire)){
        std::uint32_t newD = (d + 1) % queue->jobsCount;
        if(queue->doIndex.compare_exchange_weak(d, newD)){
            PlatformJob* job = &queue->jobs[d];
            
            job->callback(job->data);
            
            queue->finished.fetch_add(1);
        }
        else{
            // NOTE(Dima): Value has not been changed because of spuorious failure
        }
    }
    else{
        res = 1;
    }
    
    return(res);
}

InternalFunction void PlatformWorkerThread(Platform_Job_Queue* queue){
    for(;;){
        if(PlatformDoWorkerWork(queue)){
            std::unique_lock<std::mutex> uniqueLock(queue->conditionVariableMutex);
            queue->conditionVariable.wait(uniqueLock);
        }
    }
}

PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion){
    while(queue->started.load() != queue->finished.load())
    {
        PlatformDoWorkerWork(queue);
    }
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->started.store(0, std::memory_order_relaxed);
    queue->finished.store(0, std::memory_order_relaxed);
}

static void InitJobQueue(Platform_Job_Queue* queue, int jobsCount, int threadCount){
    queue->addIndex.store(0, std::memory_order_relaxed);
    queue->doIndex.store(0, std::memory_order_relaxed);
    
    queue->started.store(0, std::memory_order_relaxed);
    queue->finished.store(0, std::memory_order_relaxed);
    
    queue->jobs = (PlatformJob*)malloc(jobsCount * sizeof(PlatformJob));
    queue->jobsCount = jobsCount;
    
    for(int jobIndex = 0; jobIndex < jobsCount; jobIndex++){
        PlatformJob* job = queue->jobs + jobIndex;
        
        job->callback = 0;
        job->data = 0;
    }
    
    queue->threads.reserve(threadCount);
    for(int threadIndex = 0;
        threadIndex < threadCount;
        threadIndex++)
    {
#if 1
        queue->threads.push_back(std::thread(PlatformWorkerThread, queue));
        queue->threads[threadIndex].detach();
#else
        std::thread newThread(PlatformWorkerThread, queue);
        newThread.detach();
#endif
    }
}

static void FreeJobQueue(Platform_Job_Queue* queue){
    if(queue->jobs){
        free(queue->jobs);
    }
    queue->jobs = 0;
    queue->threads.clear();
}

void InitDefaultPlatformAPI(Platform* api){
    InitJobQueue(&api->highPriorityQueue, 2048, 8);
    InitJobQueue(&api->lowPriorityQueue, 2048, 2);
    
    api->AddEntry = PlatformAddEntry;
    api->WaitForCompletion = PlatformWaitForCompletion;
    
    api->ReadFile = PlatformReadFile;
    api->WriteFile = PlatformWriteFile;
    api->FreeFileMemory = PlatformFreeFileMemory;
}

void FreePlatformAPI(Platform* api){
    FreeJobQueue(&api->highPriorityQueue);
    FreeJobQueue(&api->lowPriorityQueue);
}


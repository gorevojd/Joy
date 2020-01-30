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
    queue->AddMutexSTD.lock();
#else
    BeginTicketMutex(&queue->AddMutex);
#endif
    
    uint32_t oldAddIndex = queue->AddIndex.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    
    uint32_t newAddIndex = (oldAddIndex + 1) % queue->JobsCount;
    // NOTE(Dima): We should not overlap
    Assert(newAddIndex != queue->DoIndex.load(std::memory_order_acquire));
    
    platform_job* job = &queue->Jobs[oldAddIndex];
    job->Callback = callback;
    job->Data = data;
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->AddIndex.store(newAddIndex, std::memory_order_relaxed);
    queue->Started.fetch_add(1);
    
    queue->ConditionVariable.notify_all();
#if PLATFORM_USE_STD_MUTEX
    queue->AddMutexSTD.unlock();
#else
    EndTicketMutex(&queue->AddMutex);
#endif
}

InternalFunction b32 PlatformDoWorkerWork(platform_job_queue* queue){
    b32 res = 0;
    
    std::uint32_t d = queue->DoIndex.load();
    
    if(d != queue->AddIndex.load(std::memory_order_acquire)){
        std::uint32_t newD = (d + 1) % queue->JobsCount;
        if(queue->DoIndex.compare_exchange_weak(d, newD)){
            platform_job* job = &queue->Jobs[d];
            
            job->Callback(job->Data);
            
            queue->Finished.fetch_add(1);
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

InternalFunction void PlatformWorkerThread(platform_job_queue* queue){
    for(;;){
        if(PlatformDoWorkerWork(queue)){
            std::unique_lock<std::mutex> UniqueLock(queue->ConditionVariableMutex);
            queue->ConditionVariable.wait(UniqueLock);
        }
    }
}

PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion){
    while(queue->Started.load() != queue->Finished.load())
    {
        PlatformDoWorkerWork(queue);
    }
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->Started.store(0, std::memory_order_relaxed);
    queue->Finished.store(0, std::memory_order_relaxed);
}

static void InitJobQueue(platform_job_queue* queue, int jobsCount, int threadCount){
    queue->AddIndex.store(0, std::memory_order_relaxed);
    queue->DoIndex.store(0, std::memory_order_relaxed);
    
    queue->Started.store(0, std::memory_order_relaxed);
    queue->Finished.store(0, std::memory_order_relaxed);
    
    queue->Jobs = (platform_job*)malloc(jobsCount * sizeof(platform_job));
    queue->JobsCount = jobsCount;
    
    for(int jobIndex = 0; jobIndex < jobsCount; jobIndex++){
        platform_job* job = queue->Jobs + jobIndex;
        
        job->Callback = 0;
        job->Data = 0;
    }
    
    queue->Threads.reserve(threadCount);
    for(int threadIndex = 0;
        threadIndex < threadCount;
        threadIndex++)
    {
#if 1
        queue->Threads.push_back(std::thread(PlatformWorkerThread, queue));
        queue->Threads[threadIndex].detach();
#else
        std::thread newThread(PlatformWorkerThread, queue);
        newThread.detach();
#endif
    }
}

static void FreeJobQueue(platform_job_queue* queue){
    if(queue->Jobs){
        free(queue->Jobs);
    }
    queue->Jobs = 0;
    queue->Threads.clear();
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


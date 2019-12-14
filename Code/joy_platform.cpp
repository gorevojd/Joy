#include "joy_platform.h"

#define PLATFORM_USE_STD_MUTEX 1

PLATFORM_READ_FILE(PlatformReadFile) {
	platform_read_file_result Result = {};
    
	FILE* fp = fopen(FilePath, "rb");
    
	if (fp) {
		fseek(fp, 0, 2);
		u64 FileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		Result.DataSize = FileSize;
		Result.Data = (u8*)calloc(FileSize + 1, 1);
        
		fread(Result.Data, 1, FileSize, fp);
        
		fclose(fp);
	}
    
	return(Result);
}

PLATFORM_WRITE_FILE(PlatformWriteFile) {
	FILE* File = (FILE*)fopen(FilePath, "wb");
    
    b32 Result = 0;
    
	if (File) {
		size_t ElementsWritten = fwrite(Data, 1, Size, File);
        
        Result = (ElementsWritten == Size);
        
		fclose(File);
	}
    
    return(Result);
}

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory) {
	if (FileReadResult->Data) {
		free(FileReadResult->Data);
	}
	FileReadResult->Data = 0;
}

PLATFORM_ADD_ENTRY(PlatformAddEntry){
#if PLATFORM_USE_STD_MUTEX
    Queue->AddMutexSTD.lock();
#else
    BeginTicketMutex(&Queue->AddMutex);
#endif
    
    uint32_t OldAddIndex = Queue->AddIndex.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    
    uint32_t NewAddIndex = (OldAddIndex + 1) % Queue->JobsCount;
    // NOTE(Dima): We should not overlap
    Assert(NewAddIndex != Queue->DoIndex.load(std::memory_order_acquire));
    
    platform_job* Job = &Queue->Jobs[OldAddIndex];
    Job->Callback = Callback;
    Job->Data = Data;
    
    std::atomic_thread_fence(std::memory_order_release);
    Queue->AddIndex.store(NewAddIndex, std::memory_order_relaxed);
    Queue->Started.fetch_add(1);
    
    Queue->ConditionVariable.notify_all();
#if PLATFORM_USE_STD_MUTEX
    Queue->AddMutexSTD.unlock();
#else
    EndTicketMutex(&Queue->AddMutex);
#endif
}

InternalFunction b32 PlatformDoWorkerWork(platform_job_queue* Queue){
    b32 Result = 0;
    
    std::uint32_t D = Queue->DoIndex.load();
    
    if(D != Queue->AddIndex.load(std::memory_order_acquire)){
        std::uint32_t NextDoIndex = (D + 1) % Queue->JobsCount;
        if(Queue->DoIndex.compare_exchange_weak(D, NextDoIndex)){
            platform_job* Job = &Queue->Jobs[D];
            
            Job->Callback(Job->Data);
            
            Queue->Finished.fetch_add(1);
        }
        else{
            // NOTE(Dima): Value has not been changed because of spuorious failure
        }
    }
    else{
        Result = 1;
    }
    
    return(Result);
}

InternalFunction void PlatformWorkerThread(platform_job_queue* Queue){
    for(;;){
        if(PlatformDoWorkerWork(Queue)){
            std::unique_lock<std::mutex> UniqueLock(Queue->ConditionVariableMutex);
            Queue->ConditionVariable.wait(UniqueLock);
        }
    }
}

PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion){
    while(Queue->Started.load() != Queue->Finished.load())
    {
        PlatformDoWorkerWork(Queue);
    }
    
    std::atomic_thread_fence(std::memory_order_release);
    Queue->Started.store(0, std::memory_order_relaxed);
    Queue->Finished.store(0, std::memory_order_relaxed);
}

static void InitJobQueue(platform_job_queue* Queue, int JobsCount, int ThreadCount){
    Queue->AddIndex.store(0, std::memory_order_relaxed);
    Queue->DoIndex.store(0, std::memory_order_relaxed);
    
    Queue->Started.store(0, std::memory_order_relaxed);
    Queue->Finished.store(0, std::memory_order_relaxed);
    
    Queue->Jobs = (platform_job*)malloc(JobsCount * sizeof(platform_job));
    Queue->JobsCount = JobsCount;
    
    for(int JobIndex = 0; JobIndex < JobsCount; JobIndex++){
        platform_job* Job = Queue->Jobs + JobIndex;
        
        Job->Callback = 0;
        Job->Data = 0;
    }
    
    Queue->Threads.reserve(ThreadCount);
    for(int ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ThreadIndex++)
    {
#if 1
        Queue->Threads.push_back(std::thread(PlatformWorkerThread, Queue));
        Queue->Threads[ThreadIndex].detach();
#else
        std::thread Thread(PlatformWorkerThread, Queue);
        Thread.detach();
#endif
    }
}

static void FreeJobQueue(platform_job_queue* Queue){
    if(Queue->Jobs){
        free(Queue->Jobs);
    }
    Queue->Jobs = 0;
    Queue->Threads.clear();
}

void InitDefaultPlatformAPI(platform_api* API){
    InitJobQueue(&API->HighPriorityQueue, 2048, 8);
    InitJobQueue(&API->LowPriorityQueue, 2048, 2);
    
    API->AddEntry = PlatformAddEntry;
    API->WaitForCompletion = PlatformWaitForCompletion;
    
    API->ReadFile = PlatformReadFile;
    API->WriteFile = PlatformWriteFile;
    API->FreeFileMemory = PlatformFreeFileMemory;
}

void FreePlatformAPI(platform_api* API){
    FreeJobQueue(&API->HighPriorityQueue);
    FreeJobQueue(&API->LowPriorityQueue);
}


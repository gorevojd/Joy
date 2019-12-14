#ifndef JOY_PLATFORM_H
#define JOY_PLATFORM_H

#include "joy_types.h"
#include "joy_defines.h"

#include <intrin.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>

#if defined(_WIN32) || defined(_WIN64)

#define PLATFORM_WINDOWS
#if defined(_WIN64)
#define PLATFORM_WINDOWS_X64
#else
#define PLATFORM_WINDOWS_X32
#endif

#elif defined(unix) || defined(__unix) || defined(__unix__)

#define PLATFORM_UNIX_BASED
#if defined(__linux__)
#define PLATFORM_LINUX
#elif defined(__APPLE__) || defined(__MACH__)
#define PLATFORM_MACOSX
#endif

#endif


#if defined(PLATFORM_WINDOWS)
#if defined(PLATFORM_WINDOWS_X64)
inline u16 GetThreadID(){
    __int64 Result = __readgsqword(0x48);
    
    return((u16)Result);
}
#else
inline u16 GetThreadID(){
    unsigned long Result = __readfsdword(0x24);
    
    return((u16)Result);
}
#endif

#else
// NOTE(Dima): Other platforms
#endif


struct platform_read_file_result{
    void* Data;
    u64 DataSize;
};

struct ticket_mutex{
    std::atomic_uint64_t Acquire;
    std::atomic_uint64_t Release;
};

inline void BeginTicketMutex(ticket_mutex* Mutex){
    uint64_t Before = Mutex->Acquire.fetch_add(1);
    while(Before != Mutex->Release.load(std::memory_order::memory_order_acquire)){
        _mm_pause();
    }
}

inline void EndTicketMutex(ticket_mutex* Mutex){
    Mutex->Release.fetch_add(1, std::memory_order::memory_order_release);
}

#define PLATFORM_READ_FILE(name) platform_read_file_result name(char* FilePath)
typedef PLATFORM_READ_FILE(platform_read_file);
PLATFORM_READ_FILE(PlatformReadFile);

#define PLATFORM_WRITE_FILE(name) b32 name(char* FilePath, void* Data, u64 Size)
typedef PLATFORM_WRITE_FILE(platform_write_file);
PLATFORM_WRITE_FILE(PlatformWriteFile);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(platform_read_file_result* FileReadResult)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);
PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory);

enum platform_error_type{
    PlatformError_Error,
    PlatformError_Warning,
    PlatformError_Information,
};

#define PLATFORM_SHOW_ERROR(name) void name(u32 Type, char* Text)
typedef PLATFORM_SHOW_ERROR(platform_show_error);

#define PLATFORM_DEBUG_OUTPUT_STRING(name) void name(char* Text)
typedef PLATFORM_DEBUG_OUTPUT_STRING(platform_debug_output_string);

#define PLATFORM_CALLBACK(name) void name(void* Data)
typedef PLATFORM_CALLBACK(platform_callback);

struct platform_job{
    platform_callback* Callback;
    void* Data;
};

struct platform_job_queue{
    ticket_mutex AddMutex;
    std::mutex AddMutexSTD;
    
    std::atomic_uint32_t AddIndex;
    std::atomic_uint32_t DoIndex;
    
    std::atomic_uint64_t Started;
    std::atomic_uint64_t Finished;
    
    platform_job* Jobs;
    int JobsCount;
    
    std::mutex ConditionVariableMutex;
    std::condition_variable ConditionVariable;
    
    std::vector<std::thread> Threads;
};

#define PLATFORM_ADD_ENTRY(name) void name(platform_job_queue* Queue, platform_callback* Callback, void* Data)
typedef PLATFORM_ADD_ENTRY(platform_add_entry);
PLATFORM_ADD_ENTRY(PlatformAddEntry);

#define PLATFORM_WAIT_FOR_COMPLETION(name) void name(platform_job_queue* Queue)
typedef PLATFORM_WAIT_FOR_COMPLETION(platform_wait_for_completion);
PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion);

struct platform_api{
    platform_read_file* ReadFile;
    platform_write_file* WriteFile;
    platform_free_file_memory* FreeFileMemory;
    
    platform_show_error* ShowError;
    platform_debug_output_string* OutputString;
    
    platform_job_queue HighPriorityQueue;
    platform_job_queue LowPriorityQueue;
    
    platform_add_entry* AddEntry;
    platform_wait_for_completion* WaitForCompletion;
};

/* Extern variables definition */
extern platform_api PlatformAPI;

void InitDefaultPlatformAPI(platform_api* API);
void FreePlatformAPI(platform_api* API);

#endif
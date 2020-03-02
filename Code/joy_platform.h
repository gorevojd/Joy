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
    __int64 result = __readgsqword(0x48);
    
    return((u16)result);
}
#else
inline u16 GetThreadID(){
    unsigned long result = __readfsdword(0x24);
    
    return((u16)result);
}
#endif

#else
// NOTE(Dima): Other platforms
#endif

struct Platform_Read_File_Result{
    void* data;
    u64 dataSize;
};

struct ticket_mutex{
    std::atomic_uint64_t acquire;
    std::atomic_uint64_t release;
};

inline void BeginTicketMutex(ticket_mutex* mutex){
    uint64_t before = mutex->acquire.fetch_add(1);
    while(before != mutex->release.load(std::memory_order::memory_order_acquire)){
        _mm_pause();
    }
}

inline void EndTicketMutex(ticket_mutex* Mutex){
    Mutex->release.fetch_add(1, std::memory_order::memory_order_release);
}

#define PLATFORM_READ_FILE(name) Platform_Read_File_Result name(char* filePath)
typedef PLATFORM_READ_FILE(Platform_Read_File);
PLATFORM_READ_FILE(PlatformReadFile);

#define PLATFORM_WRITE_FILE(name) b32 name(char* filePath, void* data, u64 size)
typedef PLATFORM_WRITE_FILE(Platform_Write_File);
PLATFORM_WRITE_FILE(PlatformWriteFile);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(Platform_Read_File_Result* fileReadResult)
typedef PLATFORM_FREE_FILE_MEMORY(Platform_Free_File_Memory);
PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory);

enum Platform_Error_Type{
    PlatformError_Error,
    PlatformError_Warning,
    PlatformError_Information,
};

#define PLATFORM_SHOW_ERROR(name) void name(u32 type, char* text)
typedef PLATFORM_SHOW_ERROR(Platform_Show_Error);

#define PLATFORM_DEBUG_OUTPUT_STRING(name) void name(char* text)
typedef PLATFORM_DEBUG_OUTPUT_STRING(Platform_Debug_Output_String);

#define PLATFORM_CALLBACK(name) void name(void* data)
typedef PLATFORM_CALLBACK(Platform_Callback);

struct platform_job{
    Platform_Callback* Callback;
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

#define PLATFORM_MEMALLOC(name) mem_block* name(mi Size)
typedef PLATFORM_MEMALLOC(platform_memalloc);

#define PLATFORM_MEMFREE(name) void name(mem_block* ToFree)
typedef PLATFORM_MEMFREE(platform_memfree);

#define PLATFORM_MEMZERO(name) void name(mem_block* ToZero)
typedef PLATFORM_MEMZERO(platform_memzero);

#define PLATFORM_ADD_ENTRY(name) void name(platform_job_queue* queue, Platform_Callback* callback, void* data)
typedef PLATFORM_ADD_ENTRY(Platform_Add_Entry);
PLATFORM_ADD_ENTRY(PlatformAddEntry);

#define PLATFORM_WAIT_FOR_COMPLETION(name) void name(platform_job_queue* queue)
typedef PLATFORM_WAIT_FOR_COMPLETION(Platform_Wait_For_Completion);
PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion);

#define PLATFORM_BEGIN_LIST_FILES_IN_DIR(name) Loaded_Strings name(char* DirectoryPath, char* Wildcard)
typedef PLATFORM_BEGIN_LIST_FILES_IN_DIR(Platform_Begin_List_Files_In_Dir);

#define PLATFORM_END_LIST_FILES_IN_DIR(name) void name(Loaded_Strings* Strings)
typedef PLATFORM_END_LIST_FILES_IN_DIR(Platform_End_List_Files_In_Dir);


enum Platform_File_Flags{
    File_Archive = 1,
    File_Compressed = 2,
    File_Directory = 4,
    File_Hidden = 8,
    File_Normal = 16,
    File_Readonly = 32,
    File_System = 64,
};

struct platform_file_desc{
    char Name[256];
    char FullPath[256];
    
    u64 Size;
    
    u32 Flags;
};

#define PLATFORM_OPEN_FILES_BEGIN(name) void name(char* DirectoryPath, char* Wildcard)
#define PLATFORM_OPEN_FILES_END(name) void name()
#define PLATFORM_OPEN_NEXT_FILE(name) b32 name(platform_file_desc* OutFile)
#define PLATFORM_FILE_OFFSET_READ(name) b32 name(char* FilePath, u64 Offset, u32 ReadCount, void* ReadTo)

typedef PLATFORM_OPEN_FILES_BEGIN(platform_open_files_begin);
typedef PLATFORM_OPEN_FILES_END(platform_open_files_end);
typedef PLATFORM_OPEN_NEXT_FILE(platform_open_next_file);
typedef PLATFORM_FILE_OFFSET_READ(platform_file_offset_read);

#define PLATFORM_PROCESS_INPUT(name) void name(struct input_state* Input)
typedef PLATFORM_PROCESS_INPUT(platform_process_input);

struct Platform{
    Platform_Read_File* ReadFile;
    Platform_Write_File* WriteFile;
    Platform_Free_File_Memory* FreeFileMemory;
    
    Platform_Show_Error* ShowError;
    Platform_Debug_Output_String* OutputString;
    
    platform_job_queue highPriorityQueue;
    platform_job_queue lowPriorityQueue;
    
    platform_memalloc* MemAlloc;
    platform_memfree* MemFree;
    platform_memzero* MemZero;
    
    platform_open_files_begin* OpenFilesBegin;
    platform_open_files_end* OpenFilesEnd;
    platform_open_next_file* OpenNextFile;
    platform_file_offset_read* FileOffsetRead;
    
    Platform_Begin_List_Files_In_Dir* BeginListFilesInDir;
    Platform_End_List_Files_In_Dir* EndListFilesInDir;
    
    Platform_Add_Entry* AddEntry;
    Platform_Wait_For_Completion* WaitForCompletion;
    
    platform_process_input* ProcessInput;
};

/* Extern variables definition */
extern Platform platform;

#endif
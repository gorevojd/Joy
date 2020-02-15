#ifndef JOY_PROFILER_INTERFACE_H
#define JOY_PROFILER_INTERFACE_H

#include <thread>
#include <atomic>

#include "joy_math.h"
#include "joy_types.h"

#define PROF_ENABLED 1

#if PROF_ENABLED

enum prof_record_type {
	ProfRecord_None,
    
	ProfRecord_BeginTiming,
	ProfRecord_EndTiming,
	ProfRecord_FrameBarrier,
};

struct prof_record {
	char* Name;
	char* UniqueName;
    u64 Clocks;
	
	u32 RecordType;
	u16 ThreadID;
    
    float Value;
};

#define PROF_LOGS_COUNT 1024
#define PROF_LOG_SIZE 1024
#define PROF_RECORD_MAX_COUNT 65536 * 16
struct prof_record_table {
    std::atomic_int Record_Table_Index;
    
	prof_record Records[2][PROF_RECORD_MAX_COUNT];
    std::atomic_int Increment;
    
	char Logs[PROF_LOGS_COUNT][PROF_LOG_SIZE];
	u32 LogsTypes[PROF_LOGS_COUNT];
	b32 LogsInited[PROF_LOGS_COUNT];
    std::atomic_int CurrentLogIndex;
    std::atomic_int LogIncrement;
};
extern prof_record_table* GlobalRecordTable;

#define PROF_LAYER_TABLE_MASK 0x40000000
#define PROF_LAYER_INDEX_MASK 0x3FFFFFFF

inline prof_record* ProfAddRecord(char* Name, char* UniqueName, u32 RecordType) {
	int Index = GlobalRecordTable->Record_Table_Index.fetch_add(GlobalRecordTable->Increment);
    
	int TableIndex = ((Index & PROF_LAYER_TABLE_MASK) != 0);
	int RecordIndex = (Index & PROF_LAYER_INDEX_MASK);
	Assert(RecordIndex < PROF_RECORD_MAX_COUNT);
    
	prof_record* Record = GlobalRecordTable->Records[TableIndex] + RecordIndex;
    
	Record->Name = Name;
	Record->UniqueName = UniqueName;
	Record->Clocks = __rdtsc();
	Record->RecordType = RecordType;
	Record->ThreadID = GetThreadID();
    
	return(Record);
}

#define PROF_FRAME_BARRIER(delta) {prof_record* Rec = ProfAddRecord("FrameBarrier", PROF_UNIQUE_STRING("FrameBarrier"), ProfRecord_FrameBarrier); Rec->Value = delta;}

struct prof_timing {
	char* Name;
	char* UniqueName;
    
	prof_timing(char* Name, char* UniqueName) {
		this->Name = Name;
		this->UniqueName = UniqueName;
        
		ProfAddRecord(Name, UniqueName, ProfRecord_BeginTiming);
	}
    
	~prof_timing() {
		ProfAddRecord(Name, UniqueName, ProfRecord_EndTiming);
	}
};

void ProfAddLog(char* Text, char* File, int Line, u32 LogType);

#define PROF_LOG(log) ProfAddLog(log, __FILE__, __LINE__, ProfLog_Log)
#define PROF_ERROR_LOG(log) ProfAddLog(log, __FILE__, __LINE__, ProfLog_ErrLog)
#define PROF_OK_LOG(log) ProfAddLog(log, __FILE__, __LINE__, ProfLog_OkLog)
#define PROF_WARN_LOG(log) ProfAddLog(log, __FILE__, __LINE__, ProfLog_WarnLog)

inline void ProfSetRecording(b32 Recording) {
    GlobalRecordTable->Increment.store(Recording);
}

inline void ProfSetLogRecording(b32 Recording) {
    GlobalRecordTable->LogIncrement.store(Recording);
}


#define PROF_ID_TO_STRING(id) #id

#define PROF_UNIQUE_STRING_(id, func, line, counter) id "@" func "@" ## PROF_ID_TO_STRING(line) ## "@" ## PROF_ID_TO_STRING(counter)
#define PROF_UNIQUE_STRING(id) PROF_UNIQUE_STRING_(id, __FUNCTION__, __LINE__, __COUNTER__)
#define PROF_UNIQUE_STRING2(id) PROF_UNIQUE_STRING_(id, __FUNCTION__, 123456, 123)


#define ADD_PROF_RECORD(name, type) ProfAddRecord(name, PROF_UNIQUE_STRING(name), type)

#define BEGIN_TIMING(name) ADD_PROF_RECORD(name, ProfRecord_BeginTiming)
#define BEGIN_REPEATED_TIMING(name)  ProfAddRecord(name, PROF_UNIQUE_STRING2(name), ProfRecord_BeginTiming)
#define END_TIMING() ADD_PROF_RECORD("End", ProfRecord_EndTiming)
#define FUNCTION_TIMING() prof_timing FunctionTiming_##__COUNTER__(__FUNCTION__, PROF_UNIQUE_STRING(__FUNCTION__))

#else // NOTE(Dima): PROF_ENABLED


#define ADD_PROF_RECORD(...)

#define BEGIN_TIMING(...)
#define BEGIN_REPEATED_TIMING(...)
#define END_TIMING(...)
#define FUNCTION_TIMING(...)

#define PROF_LOG(...)
#define PROF_ERROR_LOG(...)
#define PROF_OK_LOG(...)
#define PROF_WARN_LOG(...)

#define PROF_FRAME_BARRIER(...)


#endif // NOTE(Dima): PROF_ENABLED

#endif
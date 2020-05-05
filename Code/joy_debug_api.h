#ifndef JOY_DEBUG_API_H
#define JOY_DEBUG_API_H

#if defined(JOY_DEBUG_BUILD)


struct debug_primitive_data_line{
    v3 From;
    v3 To;
};

struct debug_primitive_data_cross{
    v3 CenterP;
    f32 Size;
};

struct debug_primitive_data_circle{
    v3 CenterP;
    f32 Radius;
    // NOTE(Dima): 0 - X, 1 - Y, 2 - Z
    u8 DirectionIndicator;
};

struct debug_primitive_data_axes{
    v3 CenterP;
    v3 Left;
    v3 Up;
    v3 Front;
    float Size;
};

struct debug_primitive_data_triangle{
    v3 P0;
    v3 P1;
    v3 P2;
};

struct debug_primitive_data_aabb{
    v3 Min;
    v3 Max;
};

struct debug_primitive_data_obb{
    v3 CenterP;
    v3 Left;
    v3 Up;
    v3 Front;
    v3 ScaleXYZ;
};

enum debug_primitive_type{
    DebugPrimitive_Line,
    DebugPrimitive_Cross,
    DebugPrimitive_Circle,
    DebugPrimitive_Axes,
    DebugPrimitive_Triangle,
    DebugPrimitive_AABB,
    DebugPrimitive_OBB,
    DebugPrimitive_String,
};

struct debug_primitive{
    v3 Color;
    f32 Duration;
    b32 DepthEnabled;
    f32 LineWidth;
    
    u32 Type;
    
    union{
        debug_primitive_data_line Line;
        debug_primitive_data_cross Cross;
        debug_primitive_data_circle Circle;
        debug_primitive_data_axes Axes;
        debug_primitive_data_triangle Triangle;
        debug_primitive_data_aabb AABB;
        debug_primitive_data_obb OBB;
    };
    
    debug_primitive* Next;
    debug_primitive* Prev;
};

enum debug_record_type{
    DebugRecord_BeginTiming = (1 << 1),
    DebugRecord_EndTiming = (1 << 2),
    DebugRecord_FrameBarrier = (1 << 3),
};

struct debug_record{
    char* UniqueName;
    u64 TimeStampCounter;
    u16 ThreadID;
    u8 Type;
};

#define DEBUG_ID_TO_STRING(name) #name
#define DEBUG_UNIQUE_SYMBOL_HELPER(a, b, c, d) a "|" b "|" c "|" d
#define DEBUG_UNIQUE_SYMBOL(name) DEBUG_UNIQUE_SYMBOL_HELPER(name, __FUNCTION__, DEBUG_ID_TO_STRING(__LINE__), DEBUG_ID_TO_STRING(__COUNTER__))

void DEBUGSetMenuDataSource(u32 Type, void* Data);

void DEBUGAddLine(v3 From, v3 To,
                  v3 Color,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddCross(v3 CenterP, v3 Color,
                   f32 Size = 1.0f,
                   f32 Duration = 0.0f,
                   b32 DepthEnabled = true);

void DEBUGAddSphere(v3 CenterP, v3 Color,
                    f32 Radius = 1.0f,
                    f32 Duration = 0.0f,
                    b32 DepthEnabled = true);

void DEBUGAddCircleX(v3 CenterP, v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddCircleY(v3 CenterP, v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddCircleZ(v3 CenterP, v3 Color,
                     f32 Radius = 1.0f,
                     f32 Duration = 0.0f,
                     b32 DepthEnabled = true);

void DEBUGAddAxes(const m44& Tfm,
                  f32 Size = 1.0f,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddTriangle(v3 Point0, v3 Point1,
                      v3 Point2, v3 Color,
                      f32 Duration = 0.0f,
                      b32 DepthEnabled = true);

void DEBUGAddAABB(v3 Min, v3 Max, v3 Color,
                  f32 Duration = 0.0f,
                  b32 DepthEnabled = true);

void DEBUGAddOBB(const m44& CenterTransform,
                 v3 ScaleXYZ,
                 v3 Color,
                 f32 Duration = 0.0f,
                 b32 DepthEnabled = true);

void DEBUGAddString(v3 P, char* Text, v3 Color,
                    f32 Size = 1.0f,
                    f32 Duration = 0.0f,
                    b32 DepthEnabled = true);


#define DEBUG_MENU_GUI_FUNC_NAME(type) DEBUGMenuFunc_##type
#define DEBUG_MENU_GUI_FUNC_CALLBACK(name) void name(struct gui_state* Gui, void* Data)
typedef DEBUG_MENU_GUI_FUNC_CALLBACK(debug_menu_gui_func_callback);

enum debug_menu_type{
    DebugMenu_Render,
    DebugMenu_Animation,
    DebugMenu_Input,
    DebugMenu_GUI,
    DebugMenu_Assets,
    DebugMenu_Platform,
    DebugMenu_Game,
    DebugMenu_DEBUG,
    DebugMenu_Profile,
    DebugMenu_Console,
    DebugMenu_Log,
    
    DebugMenu_Count,
};

struct debug_menu{
    char Name[256];
    
    void* Data;
    debug_menu_gui_func_callback* Callback;
    
    u32 Type;
};


#define DEBUG_DEFAULT_RECORDING true
#define DEBUG_TABLE_INDEX_MASK 0x80000000
#define DEBUG_TABLE_INDEX_BITSHIFT 31
#define DEBUG_RECORD_INDEX_MASK 0x7FFFFFFF

struct debug_global_table{
    mem_region* Region;
    
    void* MenuDataSources[DebugMenu_Count];
    
#define DEBUG_CIRCLE_SEGMENTS 16
    v3 CircleVerticesX[DEBUG_CIRCLE_SEGMENTS];
    v3 CircleVerticesY[DEBUG_CIRCLE_SEGMENTS];
    v3 CircleVerticesZ[DEBUG_CIRCLE_SEGMENTS];
    
    debug_primitive PrimitiveUse;
    debug_primitive PrimitiveFree;
    
    // NOTE(Dima): Debug records
    std::atomic_uint RecordAndTableIndex;
    std::atomic_uint Increment;
    
    std::atomic_uint RecordFilter;
    
    debug_record* RecordTables[2];
    int TableMaxRecordCount;
};

extern debug_global_table* DEBUGGlobalTable;

inline void DEBUGSetRecordFilter(u32 FilterMask){
    DEBUGGlobalTable->RecordFilter.store(FilterMask); 
}

inline void DEBUGUnsetRecordFilter(){
    DEBUGGlobalTable->RecordFilter.store(0xFFFFFFFF);
}

inline void DEBUGSetIncrement(b32 Enabled){
    DEBUGGlobalTable->Increment.store(Enabled ? 1 : 0);
}

inline void DEBUGAddRecord(char* UniqueName, u8 Type){
    if(DEBUGGlobalTable->RecordFilter & Type){
        u32 ToParseIndex = DEBUGGlobalTable->RecordAndTableIndex.fetch_add(DEBUGGlobalTable->Increment);
        
        u32 TableIndex = (ToParseIndex & DEBUG_TABLE_INDEX_MASK) >> DEBUG_TABLE_INDEX_BITSHIFT;
        u32 RecordIndex = ToParseIndex & DEBUG_RECORD_INDEX_MASK;
        
        debug_record* TargetArray = DEBUGGlobalTable->RecordTables[TableIndex];
        debug_record* TargetRecord = &TargetArray[RecordIndex];
        
        TargetRecord->UniqueName = UniqueName;
        TargetRecord->Type = Type;
        TargetRecord->TimeStampCounter = __rdtsc();
        TargetRecord->ThreadID = GetThreadID();
    }
}

void DEBUGParseNameFromUnique(char* To, int ToSize, char* From);
void FillAndSortStats(struct debug_state* State, 
                      struct debug_profiled_frame* Frame, 
                      b32 IncludingChildren);

struct debug_timing{
    debug_timing(char* UniqueName){
        DEBUGAddRecord(UniqueName, DebugRecord_BeginTiming);
    }
    
    ~debug_timing(){
        DEBUGAddRecord("End", DebugRecord_EndTiming);
    }
};

#define FRAME_UPDATE_NODE_NAME "FrameUpdate"
#define BEGIN_TIMING(name) DEBUGAddRecord(name, DebugRecord_BeginTiming)
#define END_TIMING() DEBUGAddRecord("End", DebugRecord_EndTiming)
#define FUNCTION_TIMING() debug_timing FuncTiming_##__LINE__(DEBUG_UNIQUE_SYMBOL(__FUNCTION__))
#define BLOCK_TIMING(name) debug_timing BlockTiming_##__LINE__(DEBUG_UNIQUE_SYMBOL(name))
#define FRAME_BARRIER() DEBUGAddRecord("FrameBarrier", DebugRecord_FrameBarrier)

#else

#define DEBUGSetMenuDataSource(...)
#define DEBUGAddLine(...)
#define DEBUGAddCross(...)
#define DEBUGAddSphere(...)
#define DEBUGAddCircleX(...)
#define DEBUGAddCircleY(...)
#define DEBUGAddCircleZ(...)
#define DEBUGAddAxes(...)
#define DEBUGAddTriangle(...)
#define DEBUGAddAABB(...)
#define DEBUGAddOBB(...)
#define DEBUGAddString(...)

// NOTE(Dima): Timing stuff
#define BEGIN_TIMING(...)
#define END_TIMING(...)
#define FUNCTION_TIMING(...)

#endif // NOTE(Dima): JOY_DEBUG_BUILD
#endif // NOTE(Dima): JOY_DEBUG_API_H
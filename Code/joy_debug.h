#ifndef JOY_DEBUG_H
#define JOY_DEBUG_H

#if defined(JOY_DEBUG_BUILD)

struct debug_window{
    v2 P;
    v2 Dim;
    char Name[64];
};

struct debug_timing_snapshot{
    u64 StartClockFirstEntry;
    u64 StartClock;
    u64 EndClock;
    u64 ClocksElapsed;
    u64 ClocksElapsedInChildren;
    u32 HitCount;
};

struct debug_profiled_tree_node{
    char* UniqueName;
    u32 NameID;
    
    debug_profiled_tree_node* Parent;
    debug_profiled_tree_node* ChildSentinel;
    
    debug_profiled_tree_node* NextAlloc;
    debug_profiled_tree_node* PrevAlloc;
    
    debug_profiled_tree_node* Next;
    debug_profiled_tree_node* Prev;
    
    debug_timing_snapshot TimingSnapshot;
};

struct debug_timing_stat{
    char* UniqueName;
    u32 NameID;
    
    debug_timing_stat* Next;
    debug_timing_stat* Prev;
    
    debug_timing_stat* NextInHash;
    
    struct{
        u64 ClocksElapsed;
        u64 ClocksElapsedInChildren;
        u32 HitCount;
    } Stat;
};


#define DEBUG_PROFILED_FRAMES_COUNT 256
#define DEBUG_STATS_TABLE_SIZE 128
#define DEBUG_STATS_TO_SORT_SIZE 1024

struct debug_profiled_frame{
    debug_profiled_tree_node RootTreeNodeUse;
    debug_timing_stat StatUse;
    
    debug_timing_stat* StatTable[DEBUG_STATS_TABLE_SIZE];
    
    debug_timing_stat* ToSortStats[DEBUG_STATS_TO_SORT_SIZE];
    int ToSortStatsCount;
    
    debug_profiled_tree_node* FrameUpdateNode;
    debug_profiled_tree_node* CurNode;
};

enum debug_profile_menu_type{
    DebugProfileMenu_TopClock,
    DebugProfileMenu_TopClockEx,
    DebugProfileMenu_RootNode,
};

struct debug_state{
    mem_region* Region;
    
    render_state* Render;
    gui_state* Gui;
    input_state* Input;
    
    debug_menu MenuDEBUG;
    debug_menu MenuProfile;
    debug_menu MenuLog;
    
    // NOTE(Dima): Menus stuff 
    b32 ShowDebugOverlay;
    b32 ShowDebugMenus;
    debug_window MainWindow;
    
    u32 ToShowMenuType;
    
    debug_menu Menus[DebugMenu_Count];
    
    // NOTE(Dima): Profiler stuff
    int CollationFrameIndex;
    int ViewFrameIndex;
    
    b32 TargetRecordingValue;
    b32 RecordingChangeRequested;
    
    debug_profiled_tree_node TreeNodeFree;
    debug_timing_stat StatFree;
    
    debug_profiled_frame ProfiledFrames[DEBUG_PROFILED_FRAMES_COUNT];
    
    u32 ToShowProfileMenuType;
    
    char RootNodesName[32];
    char SentinelElementsName[32];
};


inline debug_profiled_frame* 
GetFrameByIndex(debug_state* State, int FrameIndex){
    debug_profiled_frame* Frame = &State->ProfiledFrames[FrameIndex];
    
    return(Frame);
}


inline u64 GetClocksFromStat(debug_timing_stat* Stat, 
                             b32 IncludingChildren)
{
    u64 Result = Stat->Stat.ClocksElapsed;
    if(!IncludingChildren){
        Result -= Stat->Stat.ClocksElapsedInChildren;
    }
    
    return(Result);
}

#endif // NOTE(Dima): JOY_DEBUG_BUILD

#endif
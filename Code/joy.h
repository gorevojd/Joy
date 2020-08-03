#ifndef JOY_H
#define JOY_H

#include "joy_defines.h"
#include "joy_types.h"
#include "joy_math.h"
#include "joy_strings.h"
#include "joy_random.h"
#include "joy_simd.h"

#include "joy_platform.h"
#include "joy_memory.h"
#include "joy_debug_api.h"
#include "joy_colors.h"

struct task_data{
    task_data* Next;
    task_data* Prev;
    
    mem_region Region;
};

struct task_data_pool{
    int FreeTasksCount;
    int TotalTasksCount;
    
    platform_mutex Mutex;
    
    mi MemUsed;
    
    task_data UseSentinel;
    task_data FreeSentinel;
};

struct game_init_params{
    int InitWindowWidth;
    int InitWindowHeight;
};

#include "joy_debug_menu_types.h"

#include "joy_input.h"
#include "joy_assets.h"
#include "joy_render_stack.h"
#include "joy_render.h"
#include "joy_assets_render.h"
#include "joy_gui.h"
#include "joy_animation.h"
#include "joy_software_renderer.h"
#include "joy_debug.h"

#define GAME_GET_MODE_STATE(state_type, state_var_name) \
state_type* state_var_name = (state_type*)Mode->ModeState;\
if(!state_var_name){\
state_var_name = PushStruct(&Mode->Memory, state_type);\
Mode->ModeState = state_var_name;\
}

#define INIT_MODES(name) void name(struct game_state* Game)
typedef INIT_MODES(init_modes);

#define INIT_MODES_FUNC_NAME GlobalInitModes
#define IMPLEMENT_INIT_MODES() INIT_MODES(INIT_MODES_FUNC_NAME)

INIT_MODES(INIT_MODES_FUNC_NAME);

#define GAME_MODE_UPDATE(name) void name(struct game_state* Game, struct game_mode* Mode)
typedef GAME_MODE_UPDATE(game_mode_update_prototype);

struct game_mode{
    char Name[128];
    game_mode_update_prototype* Update;
    
    mem_region Memory;
    
    void* ModeState;
    
    int NextModeIndex;
};

struct game_state{
#define MAX_GAME_MODE_COUNT 128
    game_mode Modes[MAX_GAME_MODE_COUNT];
    int ModesCount;
    
    int CurrentModeIndex;
    
    // NOTE(Dima): Engine systems
    mem_region GuiMemory;
    mem_region InputMemory;
    mem_region RenderMemory;
    mem_region AssetsMemory;
    mem_region AnimMemory;
#if defined(JOY_INTERNAL)
    mem_region DEBUGMemory;
#endif
    
    gui_state* Gui;
    input_state* Input;
    render_state* Render;
    asset_system* Assets;
    anim_system* Anim;
#if defined(JOY_INTERNAL)
    debug_state* DEBUG;
#endif
};

// NOTE(Dima): Game API
void GameInit(game_state* Game, game_init_params Params);
void GameUpdate(game_state* Game, render_frame_info FrameInfo);
void GameFree(game_state* Game);

// NOTE(Dima): API for modes abstraction
game_mode* DescribeGameMode(game_state* Game, char* Name, game_mode_update_prototype* Update);
void SetGameMode(game_state* Game, char* ModeName);

// NOTE(Dima): Functions definitions
task_data* BeginTaskData(task_data_pool* Pool);
void EndTaskData(task_data_pool* Pool, task_data* Task);
void InitTaskDataPool(task_data_pool* Pool,
                      mem_region* Region,
                      int TasksDatasCount,
                      mi OneSize);

#endif
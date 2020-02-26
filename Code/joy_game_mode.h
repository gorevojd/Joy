#ifndef JOY_GAME_MODE_H
#define JOY_GAME_MODE_H

#include "joy_platform.h"
#include "joy_types.h"
#include "joy_strings.h"
#include "joy_memory.h"

#include "joy_render.h"
#include "joy_assets.h"
#include "joy_input.h"
#include "joy_gui.h"

#include "joy_game_interface.h"

struct platform_to_game_api{
    render_platform_api RenderAPI;
    input_platform_process* ProcessInput;
};

struct game_mode{
    char Name[128];
    
    mem_region Memory;
    
    void* ModeState;
    
    game_mode_update_prototype* Update;
    game_mode_endframe_prototype* EndFrame;
    
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
    mem_region AssetMemory;
    
    gui_state* Gui;
    input_state* Input;
    render_state* Render;
    assets* Assets;
    
    // NOTE(Dima): platform api functions
    input_platform_process* ProcessInput;
};

inline game_mode* DescribeGameMode(game_state* Game, char* Name, 
                                   game_mode_update_prototype* Update,
                                   game_mode_endframe_prototype* EndFrame)
{
    ASSERT(Game->ModesCount < MAX_GAME_MODE_COUNT);
    
    int Index = Game->ModesCount++;
    
    game_mode* Result = &Game->Modes[Index];
    
    CopyStrings(Result->Name, Name);
    
    Result->Update = Update;
    Result->EndFrame = EndFrame;
    
    return(Result);
}

void GameInit(game_state* Game, platform_to_game_api Platform2GameAPI);
void GameUpdate(game_state* Game, render_frame_info FrameInfo);
void GameFree(game_state* Game);

#endif
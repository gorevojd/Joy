#include "joy.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include "joy_memory.cpp"
#include "joy_colors.cpp"

#include "joy_input.cpp"
#include "joy_assets.cpp"
#include "joy_animation.cpp"
#include "joy_gui.cpp"
#include "joy_render.cpp"
#include "joy_debug.cpp"

#include "joy_modes.cpp"

// NOTE(Dima): task_data functions
task_data* BeginTaskData(task_data_pool* Pool)
{
    task_data* Result = 0;
    
    Platform.LockMutex(&Pool->Mutex);
    if(Pool->FreeTasksCount)
    {
        Result = Pool->FreeSentinel.Next;
        
        DLIST_REMOVE_ENTRY(Result, Next, Prev);
        DLIST_INSERT_BEFORE_SENTINEL(Result, Pool->UseSentinel, Next, Prev);
        
        Free(&Result->Region);
        
        --Pool->FreeTasksCount;
    }
    Platform.UnlockMutex(&Pool->Mutex);
    
    return(Result);
}

void EndTaskData(task_data_pool* Pool, task_data* Task)
{
    Platform.LockMutex(&Pool->Mutex);
    
    DLIST_REMOVE_ENTRY(Task, Next, Prev);
    DLIST_INSERT_BEFORE_SENTINEL(Task, Pool->FreeSentinel, Next, Prev);
    ++Pool->FreeTasksCount;
    
    Free(&Task->Region);
    
    Platform.UnlockMutex(&Pool->Mutex);
}

void InitTaskDataPool(task_data_pool* Pool,
                      mem_region* Region,
                      int TasksDatasCount,
                      mi OneSize)
{
    Platform.InitMutex(&Pool->Mutex);
    
    Pool->UseSentinel = {};
    Pool->FreeSentinel = {};
    
    DLIST_REFLECT_PTRS(Pool->UseSentinel, Next, Prev);
    DLIST_REFLECT_PTRS(Pool->FreeSentinel, Next, Prev);
    
    Pool->FreeTasksCount = TasksDatasCount;
    Pool->TotalTasksCount = TasksDatasCount;
    
    task_data* PoolArray = PushArray(Region, task_data, TasksDatasCount);
    
    for(int NewIndex = 0;
        NewIndex < TasksDatasCount;
        NewIndex++)
    {
        task_data* Task = PoolArray + NewIndex;
        
        Task->Region = PushSplit(Region, OneSize);
        
        DLIST_INSERT_BEFORE_SENTINEL(Task, 
                                     Pool->FreeSentinel, 
                                     Next, Prev);
    }
}


// NOTE(Dima): Game stuff
INTERNAL_FUNCTION game_mode* DescribeGameMode(game_state* Game, char* Name, 
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

// NOTE(Dima): Function returns -1 if it can't find the game mode
INTERNAL_FUNCTION int FindGameMode(game_state* Game, char* ModeName){
    int Result = -1;
    
    for(int ModeIndex = 0; 
        ModeIndex < Game->ModesCount;
        ModeIndex++)
    {
        if(StringsAreEqual(Game->Modes[ModeIndex].Name, ModeName)){
            Result = ModeIndex;
            break;
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION void SetGameMode(game_state* Game, char* ModeName){
    int ModeIndex = FindGameMode(Game, ModeName);
    
    ASSERT(ModeIndex != -1);
    
    Game->CurrentModeIndex = ModeIndex;
}

// NOTE(Dima): Returns true if can advance to next game mode, false othervise
INTERNAL_FUNCTION inline b32 AdvanceGameMode(game_state* Game){
    b32 Result = 0;
    
    if(Game->CurrentModeIndex + 1 < Game->ModesCount){
        Result = 1;
    }
    
    return(Result);
}

void GameInit(game_state* Game){
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Init engine systems
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Input
    Game->InputMemory = {};
    PushMemoryStruct(&Game->InputMemory, input_state, Game->Input, Region);
    InitInput(Game->Input);
    
    // NOTE(Dima): Assets
    Game->AssetMemory = {};
    PushMemoryStruct(&Game->AssetMemory, assets, Game->Assets, Memory);
    InitAssets(Game->Assets);
    
    // NOTE(Dima): Render
    Game->RenderMemory = {};
    PushMemoryStruct(&Game->RenderMemory, render_state, Game->Render, MemRegion);
    RenderInit(Game->Render, Platform.RenderAPI);
    RenderAddStack(Game->Render, "Main", Megabytes(1));
    RenderAddStack(Game->Render, "GUI", Megabytes(1));
    RenderAddStack(Game->Render, "Meshes", Megabytes(1));
    RenderAddStack(Game->Render, "DEBUG", Megabytes(1));
    // NOTE(Dima): Init platform render stuff
    Game->Render->API.Init(Game->Assets);
    
    // NOTE(Dima): Gui
    Game->GuiMemory = {};
    PushMemoryStruct(&Game->GuiMemory, gui_state, Game->Gui, Mem);
    InitGui(Game->Gui, Game->Assets);
    
    // NOTE(Dima): Animations
    Game->AnimMemory = {};
    PushMemoryStruct(&Game->AnimMemory, anim_system, Game->Anim, Region);
    InitAnimSystem(Game->Anim);
    
    // NOTE(Dima): DEBUG
#if defined(JOY_DEBUG_BUILD)
    Game->DEBUGMemory = {};
    PushMemoryStruct(&Game->DEBUGMemory, debug_state, Game->DEBUG, Region);
    DEBUGInit(Game->DEBUG, 
              Game->Render,
              Game->Gui,
              Game->Input);
#endif
    
    // NOTE(Dima): Describing all game modes
    DescribeGameMode(Game, "Title", TitleUpdate, 0);
    DescribeGameMode(Game, "Main Menu", MainMenuUpdate, 0);
    DescribeGameMode(Game, "Changing pictures", ChangingPicturesUpdate, 0);
    DescribeGameMode(Game, "Test", TestUpdate, 0);
    
    SetGameMode(Game, "Test");
}


INTERNAL_FUNCTION void UpdateGameInput(input_state* Input){
    DEBUGSetMenuDataSource(DebugMenu_Input, Input);
    
    if(KeyWentDown(Input, Key_Control)){
        Input->CapturingMouse = !Input->CapturingMouse;
    }
    
    if(KeyWentDown(Input, Key_Escape)){
        Input->QuitRequested = true;
    }
}

void GameUpdate(game_state* Game, render_frame_info FrameInfo){
    
    game_mode* CurMode = &Game->Modes[Game->CurrentModeIndex];
    
    {
        BLOCK_TIMING("Frame: Input");
        
        UpdateGameInput(Game->Input);
        Platform.ProcessInput(Game->Input);
    }
    
    RenderBeginFrame(Game->Render);
    RenderSetFrameInfo(Game->Render, FrameInfo);
    
    gui_frame_info GuiFrameInfo = {};
    GuiFrameInfo.Stack = RenderFindStack(Game->Render, "GUI");
    GuiFrameInfo.Input = Game->Input;
    GuiFrameInfo.Width = FrameInfo.Width;
    GuiFrameInfo.Height = FrameInfo.Height;
    GuiFrameInfo.DeltaTime = FrameInfo.dt;
    
    GuiFrameBegin(Game->Gui, GuiFrameInfo);
    
    if(CurMode->Update){
        CurMode->Update(Game, CurMode);
    }
    
#if defined(JOY_DEBUG_BUILD)
    {
        BLOCK_TIMING("Frame: DEBUG");
        
        /*
        NOTE(Dima): DEBUGUpdate outputs debug geometry, so it needs to calculate before render
        and after Game Mode update.
        */
        DEBUGUpdate(Game->DEBUG);
    }
#endif
    
    {
        BLOCK_TIMING("Frame: Render");
        
        GuiFramePrepare4Render(Game->Gui);
        Game->Render->API.Render();
    }
    
    GuiFrameEnd(Game->Gui);
    RenderEndFrame(Game->Render);
    
    {
        BLOCK_TIMING("Frame: SwapBuffers");
        
        Game->Render->API.SwapBuffers();
    }
}

void GameFree(game_state* Game){
    Game->Render->API.Free();
    
    // NOTE(Dima): Freing all the memories :)
    Free(&Game->GuiMemory);
    Free(&Game->InputMemory);
    Free(&Game->RenderMemory);
    Free(&Game->AssetMemory);
    Free(&Game->AnimMemory);
    Free(&Game->DEBUGMemory);
}
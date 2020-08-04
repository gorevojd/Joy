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
#include "joy_software_renderer.cpp"
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
        
        Free(&Result->Arena);
        
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
    
    Free(&Task->Arena);
    
    Platform.UnlockMutex(&Pool->Mutex);
}

void InitTaskDataPool(task_data_pool* Pool,
                      mem_arena* Arena,
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
    
    task_data* PoolArray = PushArray(Arena, task_data, TasksDatasCount);
    
    for(int NewIndex = 0;
        NewIndex < TasksDatasCount;
        NewIndex++)
    {
        task_data* Task = PoolArray + NewIndex;
        
        Task->Arena = PushSplit(Arena, OneSize);
        
        DLIST_INSERT_BEFORE_SENTINEL(Task, 
                                     Pool->FreeSentinel, 
                                     Next, Prev);
    }
}


// NOTE(Dima): Game stuff
game_mode* DescribeGameMode(game_state* Game, char* Name, 
                            game_mode_update_prototype* Update)
{
    ASSERT(Game->ModesCount < MAX_GAME_MODE_COUNT);
    
    int Index = Game->ModesCount++;
    
    game_mode* Result = &Game->Modes[Index];
    
    CopyStrings(Result->Name, Name);
    
    Result->Update = Update;
    
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

void SetGameMode(game_state* Game, char* ModeName){
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

temp_state* TempStateInit(mem_arena* Arena, 
                          int InitWindowWidth,
                          int InitWindowHeight,
                          asset_system* Assets)
{
    temp_state* TempState = PushInMemoryStruct(Arena, temp_state, Arena);
    
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Init engine systems
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Input
    TempState->InputMemory = {};
    TempState->Input = PushInMemoryStruct(&TempState->InputMemory, input_state, Arena);
    InitInput(TempState->Input);
    
    // NOTE(Dima): Render
    TempState->RenderMemory = {};
    TempState->Render = PushInMemoryStruct(&TempState->RenderMemory, render_state, Arena);
    RenderInit(TempState->Render,
               InitWindowWidth, 
               InitWindowHeight,
               Platform.RenderAPI);
    
    // NOTE(Dima): Gui
    TempState->GuiMemory = {};
    TempState->Gui = PushInMemoryStruct(&TempState->GuiMemory, gui_state, Arena);
    InitGui(TempState->Gui, Assets);
    
    // NOTE(Dima): DEBUG
#if defined(JOY_INTERNAL)
    TempState->DEBUGMemory = {};
    TempState->DEBUG = PushInMemoryStruct(&TempState->DEBUGMemory, debug_state, Arena);
    DEBUGInit(TempState->DEBUG, 
              TempState->Render,
              TempState->Gui,
              TempState->Input);
#endif
    
    return(TempState);
}

void TempStateFree(temp_state* TempState)
{
    Free(&TempState->GuiMemory);
    Free(&TempState->InputMemory);
    Free(&TempState->RenderMemory);
#if defined(JOY_INTERNAL)
    Free(&TempState->DEBUGMemory);
#endif
}

game_state* GameStateInit(mem_arena* Arena)
{
    game_state* Game = PushInMemoryStruct(Arena, game_state, Arena);
    
    // NOTE(Dima): Assets
    Game->AssetsMemory = {};
    Game->Assets = PushInMemoryStruct(&Game->AssetsMemory, asset_system, Memory);
    InitAssets(Game->Assets);
    
    // NOTE(Dima): Animations
    Game->AnimMemory = {};
    Game->Anim = PushInMemoryStruct(&Game->AnimMemory, anim_system, Arena);
    InitAnimSystem(Game->Anim);
    
    INIT_MODES_FUNC_NAME(Game);
    
    return(Game);
}

void GameStateFree(game_state* Game)
{
    // NOTE(Dima): Freing all the memories :)
    Free(&Game->AssetsMemory);
    Free(&Game->AnimMemory);
}

INTERNAL_FUNCTION void UpdateGameInput(temp_state* TempState)
{
    input_state* Input = TempState->Input;
    render_state* Render = TempState->Render;
    
    DEBUGSetMenuDataSource(DebugMenu_Input, Input);
    
    if(KeyWentDown(Input, Key_Control)){
        Input->CapturingMouse = !Input->CapturingMouse;
    }
    
    if(KeyWentDown(Input, Key_Escape)){
        Input->QuitRequested = true;
    }
    
#if defined(JOY_INTERNAL)
    if(KeyWentDown(Input, Key_F1)){
        if(KeyIsDown(Input, Key_Shift))
        {
            if(Render->SSAOFilterType == 0)
            {
                Render->SSAOFilterType = RenderFilter_Count;
            }
            Render->SSAOFilterType = (Render->SSAOFilterType - 1) % RenderFilter_Count;
        }
        else 
        {
            Render->SSAOFilterType = (Render->SSAOFilterType + 1) % RenderFilter_Count;
        }
    }
    
    if(KeyWentDown(Input, Key_F2)){
        if(KeyIsDown(Input, Key_Shift))
        {
            if(Render->ToShowBufferType == 0)
            {
                Render->ToShowBufferType = RenderShowBuffer_Count;
            }
            Render->ToShowBufferType = (Render->ToShowBufferType - 1) % RenderShowBuffer_Count;
        }
        else 
        {
            Render->ToShowBufferType = (Render->ToShowBufferType + 1) % RenderShowBuffer_Count;
        }
    }
#endif
    
    Platform.ProcessInput(Input);
}

void UpdateGameAndEngine(game_state* Game, temp_state* TempState, render_frame_info FrameInfo)
{
    game_mode* CurMode = &Game->Modes[Game->CurrentModeIndex];
    
    asset_system* Assets = Game->Assets;
    
    render_state* Render = TempState->Render;
    input_state* Input = TempState->Input;
    gui_state* Gui = TempState->Gui;
    
    {
        BLOCK_TIMING("Frame: Input");
        
        UpdateGameInput(TempState);
    }
    
    RenderBeginFrame(Render);
    RenderSetFrameInfo(Render, FrameInfo);
    
    gui_frame_info GuiFrameInfo = {};
    GuiFrameInfo.RenderState = Render;
    GuiFrameInfo.Input = Input;
    GuiFrameInfo.Width = FrameInfo.InitWidth;
    GuiFrameInfo.Height = FrameInfo.InitHeight;
    GuiFrameInfo.DeltaTime = FrameInfo.dt;
    
    GuiFrameBegin(Gui, GuiFrameInfo);
    
    if(CurMode->Update){
        BLOCK_TIMING("Frame: GameModeUpdate");
        
        CurMode->Update(Game, TempState, CurMode);
    }
    
#if defined(JOY_INTERNAL)
    {
        BLOCK_TIMING("Frame: DEBUG");
        
        /*
        NOTE(Dima): DEBUGUpdate outputs debug geometry, so it needs to calculate before render
        and after Game Mode update.
        */
        DEBUGUpdate(TempState->DEBUG);
    }
#endif
    
    {
        BLOCK_TIMING("Frame: Render");
        
        GuiFramePrepare4Render(Gui);
        RenderEverything(Render);
    }
    
    GuiFrameEnd(Gui);
    RenderEndFrame(Render);
    
    {
        BLOCK_TIMING("Frame: SwapBuffers");
        
        RenderSwapBuffers(Render);
    }
}

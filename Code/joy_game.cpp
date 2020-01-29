#include "joy_game.h"

#include "joy_modes.h"

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

void GameInit(game_state* Game, platform_to_game_api Platform2GameAPI){
    
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Init engine systems
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Input
    Game->InputMemory = {};
    PushMemoryStruct(&Game->InputMemory, input_state, Game->Input, Region);
    
    // NOTE(Dima): Assets
    Game->AssetMemory = {};
    PushMemoryStruct(&Game->AssetMemory, assets, Game->Assets, Region);
    InitAssets(Game->Assets);
    
    // NOTE(Dima): Render
    Game->RenderMemory = {};
    PushMemoryStruct(&Game->RenderMemory, render_state, Game->Render, MemRegion);
    RenderInit(Game->Render, Platform2GameAPI.RenderAPI);
    RenderAddStack(Game->Render, "Main");
    RenderAddStack(Game->Render, "GUI");
    // NOTE(Dima): Init platform render stuff
    Game->Render->Init(Game->Assets);
    
    // NOTE(Dima): Gui
    Game->GuiMemory = {};
    PushMemoryStruct(&Game->GuiMemory, gui_state, Game->Gui, Mem);
    InitGui(Game->Gui, Game->Assets);
    
    // NOTE(Dima): Describing all game modes
    DescribeGameMode(Game, "Title", TitleUpdate, 0),
    DescribeGameMode(Game, "Main Menu", MainMenuUpdate, 0),
    DescribeGameMode(Game, "Changing pictures", ChangingPicturesUpdate, 0),
    DescribeGameMode(Game, "Test", TestUpdate, 0),
    
    SetGameMode(Game, "Test");
    
    // NOTE(Dima): Setting other things of game
    Game->ProcessInput = Platform2GameAPI.ProcessInput;
}

void GameUpdate(game_state* Game, render_frame_info FrameInfo){
    game_mode* CurMode = &Game->Modes[Game->CurrentModeIndex];
    
    Game->ProcessInput();
    
    RenderBeginFrame(Game->Render);
    RenderSetFrameInfo(Game->Render, FrameInfo);
    
    gui_frame_info GuiFrameInfo = {};
    GuiFrameInfo.Stack = RenderFindStack(Game->Render, "GUI");
    GuiFrameInfo.Input = Game->Input;
    GuiFrameInfo.Width = FrameInfo.Width;
    GuiFrameInfo.Height = FrameInfo.Height;
    
    // NOTE(Dima): Init gui for frame
    GuiFrameBegin(Game->Gui, GuiFrameInfo);
    
    // NOTE(Dima): Update game
    if(CurMode->Update){
        CurMode->Update(Game, CurMode);
    }
    
    // NOTE(Dima): Preparing GUI 4 render
    GuiFramePrepare4Render(Game->Gui);
    
    Game->Render->Render();
    
    GuiFrameEnd(Game->Gui);
    RenderEndFrame(Game->Render);
    
    Game->Render->SwapBuffers();
}

void GameFree(game_state* Game){
    Game->Render->Free();
    
    // NOTE(Dima): Freing all the memories :)
    FreeMemoryRegion(&Game->GuiMemory);
    FreeMemoryRegion(&Game->InputMemory);
    FreeMemoryRegion(&Game->RenderMemory);
    FreeMemoryRegion(&Game->AssetMemory);
}
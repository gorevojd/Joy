#include "joy_modes.h"

#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_gui.h"
#include "joy_render.h"

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    
    PushClearColor(Stack, V3(1.0f, 0.5f, 0.0f));
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    
#if 0    
    if(!Initialized){
        
        Loaded_Strings List1 = Win32BeginListFilesInDir("../Code/", "*.cpp");
        Win32EndListFilesInDir(&List1);
        Loaded_Strings List = Win32BeginListFilesInDir("../Code/", "*.h");
    }
    
    rc2 ClipRect = RcMinMax(V2(0.0f, 0.0f), V2(windowWidth, windowHeight));
    
    
#if 0        
    PushGradient(renderStack, RcMinDim(V2(10, 10), V2(900, 300)), 
                 ColorFromHex("#FF00FF"), ColorFromHex("#4b0082"),
                 RenderEntryGradient_Horizontal);
    PushGradient(renderStack, RcMinDim(V2(100, 400), V2(900, 300)), 
                 ColorFromHex("#FF00FF"), ColorFromHex("#4b0082"), 
                 RenderEntryGradient_Vertical);
#endif
    
    
#if 0
    PushBitmap(renderStack, &gAssets.sunset, V2(100.0f, Sin(time * 1.0f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.sunsetOrange, V2(200.0f, Sin(time * 1.1f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.sunsetField, V2(300.0f, Sin(time * 1.2f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.sunsetMountains, V2(400.0f, Sin(time * 1.3f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.mountainsFuji, V2(500.0f, Sin(time * 1.4f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.roadClouds, V2(600.0f, Sin(time * 1.5f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
    PushBitmap(renderStack, &gAssets.sunrise, V2(700.0f, Sin(time * 1.6f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
    
#if 1
    GuiBeginTree(&gGui, "String List");
    char ListInfo[64];
    stbsp_sprintf(ListInfo, "String list count: %d", List.Count);
    GuiText(&gGui, ListInfo);
    
    for(int i = 0; i < List.Count; i++){
        GuiText(&gGui, List.Strings[i]);
    }
    GuiEndTree(&gGui);
#endif
#endif
    
}

// NOTE(Dima): MAIN MENU GAME MODE
GAME_MODE_UPDATE(MainMenuUpdate){
    
}

struct image_swapper_state{
    float FadeoutTime;
    float ShowTime;
    int ShowIndex;
    int ShowNextIndex;
    float TimeSinceShow;
    float ShowSpeed;
    
    b32 Initialized;
};

// NOTE(Dima): Changing pictures GAME MODE
GAME_MODE_UPDATE(ChangingPicturesUpdate){
    image_swapper_state* State = (image_swapper_state*)Mode->ModeState;
    
    Bmp_Info* ToShowArray = Game->Assets->fadeoutBmps;
    int ToShowCount = Game->Assets->fadeoutBmpsCount;
    
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    
    PushClearColor(Stack, V3(1.0f, 0.5f, 0.0f));
    
    if(!State){
        State = PushStruct(&Mode->Memory, image_swapper_state);
        Mode->ModeState = State;
    }
    
    if(!State->Initialized){
        State->FadeoutTime = 1.5f;
        State->ShowTime = 5.0f;
        State->ShowIndex = 0;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
        State->ShowSpeed = 1.0f;
        
        State->Initialized = JOY_TRUE;
    }
    
    Bmp_Info* toShow = ToShowArray + State->ShowIndex;
    Bmp_Info* toShowNext = ToShowArray + State->ShowNextIndex;
    
    int Width = Game->Render->FrameInfo.Width;
    int Height = Game->Render->FrameInfo.Height;
    
    float toShowH = CalcScreenFitHeight(
        toShow->Width, toShow->Height,
        Width, Height);
    float toShowNextH = CalcScreenFitHeight(
        toShowNext->Width, toShowNext->Height,
        Width, Height);
    
    float fadeoutAlpha = Clamp01((State->TimeSinceShow - State->ShowTime) / State->FadeoutTime);
    
    PushBitmap(
        Stack, 
        toShow, 
        V2(0.0f, 0.0f), 
        toShowH, 
        V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    PushBitmap(
        Stack, 
        toShowNext, 
        V2(0.0f, 0.0f), 
        toShowNextH, 
        V4(1.0f, 1.0f, 1.0f, fadeoutAlpha));
    
    PushBitmap(Stack,
               &Game->Assets->MainLargeAtlas.Bitmap,
               V2(100, 100),
               1000,
               V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    State->TimeSinceShow += Game->Render->FrameInfo.dt * State->ShowSpeed;
    if(State->TimeSinceShow > State->ShowTime + State->FadeoutTime){
        State->ShowIndex = State->ShowNextIndex;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
    }
    
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
}

// NOTE(Dima): TITLE GAME MODE
GAME_MODE_UPDATE(TitleUpdate){
    
}

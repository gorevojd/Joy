#include "joy_modes.h"

#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_gui.h"
#include "joy_render.h"
#include "joy_camera.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

struct test_game_mode_state{
    
    game_camera Camera;
    
    float CameraSpeed;
    float MouseSencitivity;
    
    b32 Initialized;
};

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    GAME_GET_MODE_STATE(test_game_mode_state, State);
    
    
    if(!State->Initialized){
        
        State->Camera = {};
        State->CameraSpeed = 5.0f;
        State->MouseSencitivity = 0.25f;
        
        State->Initialized = JOY_TRUE;
    }
    
    float DeltaTime = Game->Render->FrameInfo.dt;
    
    // NOTE(Dima): Processing camera
    game_camera* Camera = &State->Camera;
    
    float CamSpeed = State->CameraSpeed;
    
    if(KeyIsDown(Game->Input, Key_Shift)){
        CamSpeed *= 8.0f;
    }
    
    UpdateCameraRotation(Camera, 
                         Game->Input->MouseDeltaP.y * State->MouseSencitivity, 
                         Game->Input->MouseDeltaP.x * State->MouseSencitivity,
                         0.0f);
    
    v3 MoveVector = GetMoveVector(Game->Input, -1);
    
    m33 CamTransform = (Quat2M33(Camera->Rotation));
    v3 AccVector = MoveVector * CamTransform * CamSpeed;
    
    if(SqMagnitude(MoveVector) > 0.001f){
        MoveVector = AccVector * DeltaTime;
    }
    else{
        MoveVector = Camera->dP * DeltaTime;
    }
    Camera->dP = (Camera->dP + AccVector * DeltaTime) * (1.0f - 3.0f * DeltaTime);
    
    Camera->P += MoveVector;
    
    m44 CameraTransform = GetCameraMatrix(Camera);
    
    render_pass* Pass = BeginRenderPass(Game->Render);
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    AddStackToRenderPass(Pass, Stack);
    
    int Width = Game->Render->FrameInfo.Width;
    int Height = Game->Render->FrameInfo.Height;
    
    RenderPassSetCamera(Pass, 
                        PerspectiveProjection(Width, Height, 1000.0f, 0.01f),
                        CameraTransform,
                        Width,
                        Height);
    
    PushClearColor(Stack, V3(1.0f, 0.5f, 0.0f));
    
    char CameraInfo[256];
    stbsp_sprintf(CameraInfo, "P(x%.3f; y%.3f; z%.3f)", 
                  Camera->P.x, 
                  Camera->P.y,
                  Camera->P.z);
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    GuiText(Game->Gui, CameraInfo);
    
    if(ButIsDown(Game->Input, Button_Left)){
        GuiText(Game->Gui, "Left");
    }
    
    if(ButIsDown(Game->Input, Button_Right)){
        GuiText(Game->Gui, "Right");
    }
    
    PushMesh(Stack, &Game->Assets->Cube, 
             V3(5.0f, 1.0f + Sin(Game->Input->Time * 2.0f) * 0.5f, 0.0f), 
             QuatI(), V3(1.0f));
    PushMesh(Stack, &Game->Assets->Cube, 
             V3(0.0f, 1.0f + Sin(Game->Input->Time * 3.0f) * 0.5f, 0.0f), 
             QuatI(), V3(1.0f));
    PushMesh(Stack, &Game->Assets->Cylynder, 
             V3(-10.0f, 1.0f, 0.0f), 
             Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), V3(2.0f));
    PushMesh(Stack, &Game->Assets->Sphere, 
             V3(0.0f, 1.0f + Sin(Game->Input->Time * 4.0f), 5.0f), 
             QuatI(), V3(1.0f));
    PushMesh(Stack, &Game->Assets->Plane, 
             V3(0.0f, -1.0f, 0.0f), 
             QuatI(), V3(100.0f));
    
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
    GAME_GET_MODE_STATE(image_swapper_state, State);
    
    Bmp_Info* ToShowArray = Game->Assets->fadeoutBmps;
    int ToShowCount = Game->Assets->fadeoutBmpsCount;
    
    render_pass* Pass = BeginRenderPass(Game->Render);
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    AddStackToRenderPass(Pass, Stack);
    
    PushClearColor(Stack, V3(1.0f, 0.5f, 0.0f));
    
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
    
    State->TimeSinceShow += Game->Render->FrameInfo.dt * State->ShowSpeed;
    if(State->TimeSinceShow > State->ShowTime + State->FadeoutTime){
        State->ShowIndex = State->ShowNextIndex;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
    }
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    
    PushBitmap(Stack,
               &Game->Assets->MainLargeAtlas.Bitmap,
               V2(100, 100),
               1000,
               V4(1.0f, 1.0f, 1.0f, 1.0f));
    
}

// NOTE(Dima): TITLE GAME MODE
GAME_MODE_UPDATE(TitleUpdate){
    
}

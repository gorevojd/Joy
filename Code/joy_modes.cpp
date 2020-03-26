#include "joy_modes.h"

#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_gui.h"
#include "joy_render.h"
#include "joy_camera.h"
#include "joy_data_structures.h"
#include "joy_assets_render.h"
#include "joy_random.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

struct sphere_distribution{
    int Count;
    
    int MaxCount;
    v3 *Samples;
};

// NOTE(Dima): Returns how manu distributions were actually generated
// NOTE(Dima): Should not return more than ToGenerateEstimateCount
#define GEN_SPHERE_DISTRIBUTION_CALLBACK(name) int name(int ToGenerateEstimateCount, v3* Samples)
typedef GEN_SPHERE_DISTRIBUTION_CALLBACK(gen_sphere_distribution_callback);

GEN_SPHERE_DISTRIBUTION_CALLBACK(GenFibonacciSphereDistributions){
    int Result = ToGenerateEstimateCount;
    
    float GoldenRatio = (1.0f + Sqrt(5.0f)) * 0.5f;
    float OneOverGoldenRatio = 1.0f / GoldenRatio;
    
    float N = ToGenerateEstimateCount;
    float OneOverN = 1.0f / N;
    
    for(int Index = 0;
        Index < ToGenerateEstimateCount;
        Index++)
    {
        float tX = (float)Index * OneOverN;
        float tY = (float)Index * OneOverGoldenRatio;
        
        float Theta = ACos(2.0f * tX - 1.0f) - JOY_PI_OVER_TWO;
        float Phi = JOY_TWO_PI * tY;
        
        float UnitX = cosf(Theta) * cosf(Phi);
        float UnitY = cosf(Theta) * sinf(Phi);
        float UnitZ = sinf(Theta);
        
        v3 UnitVector = V3(UnitX, UnitY, UnitZ);
        UnitVector = NOZ(UnitVector);
        
        Samples[Index] = UnitVector;
    }
    
    return(Result);
}

GEN_SPHERE_DISTRIBUTION_CALLBACK(GenTrigonometricSphereDistributions){
    int SideCount = (int)Sqrt(ToGenerateEstimateCount);
    
    int ResultCount = SideCount * SideCount;
    
    if(ResultCount){
        for(int YIndex = 0; 
            YIndex < SideCount; 
            YIndex++)
        {
            for(int XIndex = 0; 
                XIndex < SideCount;
                XIndex++)
            {
                float RandomX = RandomUni();
                float RandomY = RandomUni();
                
                float Theta = 2.0f * ACos(Sqrt(1.0f - RandomX));
                float Phi = 2.0f * JOY_PI * RandomY;
                
                float UnitX = sinf(Theta) * cosf(Phi);
                float UnitY = sinf(Theta) * sinf(Phi);
                float UnitZ = cosf(Theta);
                
                v3 UnitVector = V3(UnitX, UnitY, UnitZ);
                UnitVector = NOZ(UnitVector);
                
                int NewSampleIndex = YIndex * SideCount + XIndex;
                Samples[NewSampleIndex] = UnitVector;
            }
        }
    }
    
    return(ResultCount);
}

inline sphere_distribution 
GenerateSphereDistribution(
gen_sphere_distribution_callback* GenDistributions,
int MaxCount,
v3* TargetSamples)
{
    sphere_distribution Result = {};
    
    Result.MaxCount = MaxCount;
    Result.Samples = TargetSamples;
    if(GenDistributions){
        Result.Count = GenDistributions(MaxCount, TargetSamples);
    }
    
    return(Result);
}

struct test_game_mode_state{
    game_camera Camera;
    
    float CameraSpeed;
    float MouseSencitivity;
    
    v3* SphereDistributionsTrig;
    v3* SphereDistributionsFib;
    sphere_distribution SphereDistributionTrig;
    sphere_distribution SphereDistributionFib;
    
    b32 Initialized;
};

INTERNAL_FUNCTION void ShowSphereDistributions(game_state* Game,
                                               render_stack* Stack,
                                               sphere_distribution* Distr,
                                               u32 SphereID,
                                               v3 SphereCenter, 
                                               float SphereRad)
{
    PushOrLoadMesh(Game->Assets, Stack, 
                   SphereID, 
                   SphereCenter, 
                   QuatI(), 
                   V3(SphereRad * 2.0f),
                   ASSET_LOAD_DEFERRED);
    
    for(int SampleIndex = 0;
        SampleIndex < Distr->Count;
        SampleIndex++)
    {
        v3 TargetP = SphereCenter + Distr->Samples[SampleIndex] * SphereRad;
        
        PushOrLoadMesh(Game->Assets, Stack, 
                       GetFirst(Game->Assets, GameAsset_Cube),
                       TargetP, 
                       QuatI(), 
                       V3(0.05f),
                       ASSET_LOAD_DEFERRED);
    }
    
}

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    GAME_GET_MODE_STATE(test_game_mode_state, State);
    
    if(!State->Initialized){
        
        State->Camera = {};
        State->CameraSpeed = 5.0f;
        State->MouseSencitivity = 0.25f;
        
        int SphereDistributionsmaxCount = 1024;
        
        State->SphereDistributionsTrig = PushArray(&Mode->Memory, v3, SphereDistributionsmaxCount);
        State->SphereDistributionsFib = PushArray(&Mode->Memory, v3, SphereDistributionsmaxCount);
        
        State->SphereDistributionTrig = GenerateSphereDistribution(GenTrigonometricSphereDistributions,
                                                                   SphereDistributionsmaxCount,
                                                                   State->SphereDistributionsTrig);
        
        State->SphereDistributionFib = GenerateSphereDistribution(GenFibonacciSphereDistributions,
                                                                  SphereDistributionsmaxCount,
                                                                  State->SphereDistributionsFib);
        
        State->Initialized = JOY_TRUE;
    }
    
    float DeltaTime = Game->Render->FrameInfo.dt;
    
    // NOTE(Dima): Processing camera
    game_camera* Camera = &State->Camera;
    
    float CamSpeed = State->CameraSpeed;
    
    if(KeyIsDown(Game->Input, Key_Shift)){
        CamSpeed *= 8.0f;
    }
    
    float DeltaMouseX = GetMoveAxis(Game->Input, MoveAxis_MouseX) * State->MouseSencitivity;
    float DeltaMouseY = GetMoveAxis(Game->Input, MoveAxis_MouseY) * State->MouseSencitivity;
    
    if(Game->Input->CapturingMouse){
        
        UpdateCameraRotation(Camera, 
                             -DeltaMouseY, 
                             DeltaMouseX,
                             0.0f);
    }
    
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
    
    PushClearColor(Stack, V3(0.1f, 0.3f, 0.9f));
    
    char CameraInfo[256];
    stbsp_sprintf(CameraInfo, "P(x%.3f; y%.3f; z%.3f)", 
                  Camera->P.x, 
                  Camera->P.y,
                  Camera->P.z);
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    GuiText(Game->Gui, CameraInfo);
    
    char MouseInfo[256];
    stbsp_sprintf(MouseInfo,
                  "Delta mouse X: %.2f", 
                  DeltaMouseX);
    GuiText(Game->Gui, MouseInfo);
    stbsp_sprintf(MouseInfo, 
                  "Delta mouse Y: %.2f",
                  DeltaMouseY);
    GuiText(Game->Gui, MouseInfo);
    
    if(ButIsDown(Game->Input, Button_Left)){
        GuiText(Game->Gui, "Left");
    }
    
    if(ButIsDown(Game->Input, Button_Right)){
        GuiText(Game->Gui, "Right");
    }
    
    static float FindSphereQuality = 0.5f;
    GuiSliderFloat(Game->Gui, 
                   &FindSphereQuality, 
                   0.0f, 1.0f,
                   "LOD");
    
    u32 FindTagTypes[1] = {AssetTag_LOD};
    asset_tag_value FindTagValues[1] = {FindSphereQuality};
    
    u32 SphereID = GetBestByTags(Game->Assets,
                                 GameAsset_Sphere,
                                 FindTagTypes,
                                 FindTagValues,
                                 1);
    
    u32 CylID = GetBestByTags(Game->Assets,
                              GameAsset_Cylynder,
                              FindTagTypes,
                              FindTagValues,
                              1);
    
    int SphereLayers = 10;
    int SphereLayerCount = 10;
    v3 SphereStartP = V3(0.0f, 1.0f, -15.0f);
    for(int LayerIndex = 0; LayerIndex < SphereLayers; LayerIndex++){
        for(int InLayerIndex = 0;
            InLayerIndex < SphereLayerCount;
            InLayerIndex++)
        {
            v3 SphereP = SphereStartP + 
                V3(0.0f, 1.2f, 0.0f) * LayerIndex + 
                V3(1.2f, 0.0f, 0.0f) * InLayerIndex;
            
            
            PushOrLoadMesh(Game->Assets, Stack, 
                           SphereID,
                           SphereP, QuatI(), V3(1.0f),
                           ASSET_LOAD_DEFERRED);
        }
    }
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(5.0f, 1.0f + Sin(Game->Input->Time * 2.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_LOAD_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(0.0f, 1.0f + Sin(Game->Input->Time * 3.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_LOAD_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   CylID,
                   V3(-10.0f, 1.0f, 0.0f), 
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), V3(2.0f),
                   ASSET_LOAD_DEFERRED);
    
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cylynder),
                   V3(-13.0f, 1.0f, 0.0f),
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), 
                   V3(1.0f), 
                   ASSET_LOAD_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   SphereID,V3(0.0f, 1.0f + Sin(Game->Input->Time * 4.0f), 5.0f), 
                   QuatI(), V3(1.0f),
                   ASSET_LOAD_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Plane),
                   V3(0.0f, -1.0f, 0.0f), 
                   QuatI(), V3(100.0f),
                   ASSET_LOAD_DEFERRED);
    
    ShowSphereDistributions(Game, Stack,
                            &State->SphereDistributionTrig,
                            SphereID,
                            V3(0.0f, 10.0f, 0.0f),
                            2.0f);
    
    ShowSphereDistributions(Game, Stack,
                            &State->SphereDistributionFib,
                            SphereID,
                            V3(10.0f, 10.0f, 0.0f),
                            2.0f);
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
    
    asset_id ArrID = GetFirst(Game->Assets, GameAsset_FadeoutBmps);
    asset* Asset = GetAssetByID(Game->Assets, ArrID);
    ASSERT(Asset->Type == AssetType_Array);
    
    array_info* Arr = GET_ASSET_PTR_MEMBER(Asset, array_info);
    
    render_pass* Pass = BeginRenderPass(Game->Render);
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    AddStackToRenderPass(Pass, Stack);
    
    PushClearColor(Stack, V3(1.0f, 0.5f, 0.0f));
    
    int ToShowCount = Arr->Count;
    
    if(!State->Initialized){
        State->FadeoutTime = 1.5f;
        State->ShowTime = 5.0f;
        State->ShowIndex = 0;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
        State->ShowSpeed = 1.0f;
        
        State->Initialized = JOY_TRUE;
    }
#if 0    
    
    bmp_info* toShow = ToShowArray + State->ShowIndex;
    bmp_info* toShowNext = ToShowArray + State->ShowNextIndex;
    
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
#endif
    
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

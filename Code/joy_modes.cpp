#include "joy_modes.h"

#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_gui.h"
#include "joy_render.h"
#include "joy_camera.h"
#include "joy_assets_render.h"
#include "joy_random.h"
#include "joy_animation.h"

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
                   ASSET_IMPORT_DEFERRED);
    
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
                       ASSET_IMPORT_DEFERRED);
    }
    
}


INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitPlayerAC)
{
    anim_controller* AC = CreateAnimControl(Anim, NodesCheckSum);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(AC, AnimState_Animation, "Idle");
    AddAnimState(AC, AnimState_Animation, "Run");
    AddAnimState(AC, AnimState_Animation, "Falling");
    
#if 0
    AddAnimState(AC, AnimState_Animation, "Run1");
    AddAnimState(AC, AnimState_Animation, "Run2");
    AddAnimState(AC, AnimState_Animation, "Run3");
    AddAnimState(AC, AnimState_Animation, "Run4");
    AddAnimState(AC, AnimState_Animation, "Run5");
    AddAnimState(AC, AnimState_Animation, "Run6");
    AddAnimState(AC, AnimState_Animation, "Run7");
    AddAnimState(AC, AnimState_Animation, "Run8");
    AddAnimState(AC, AnimState_Animation, "Run9");
    AddAnimState(AC, AnimState_Animation, "Run10");
    AddAnimState(AC, AnimState_Animation, "Run11");
    AddAnimState(AC, AnimState_Animation, "Run12");
    AddAnimState(AC, AnimState_Animation, "Run13");
    AddAnimState(AC, AnimState_Animation, "Run14");
    
    anim_graph_node* FindRes1 = FindGraphNode(AC, "Run14");
    anim_graph_node* FindRes2 = FindGraphNode(AC, "Run7");
#endif
    
    // NOTE(Dima): Adding condition variables
    AddVariable(AC, "VelocityLength", 
                AnimVariable_Float);
    AddVariable(AC, "IsFalling",
                AnimVariable_Bool);
    
#if 0 
    AddVariable(AC, "TempVar0", AnimVariable_Bool);
    AddVariable(AC, "TempVar1", AnimVariable_Bool);
    AddVariable(AC, "TempVar2", AnimVariable_Bool);
    AddVariable(AC, "TempVar3", AnimVariable_Bool);
    AddVariable(AC, "TempVar4", AnimVariable_Bool);
    AddVariable(AC, "TempVar5", AnimVariable_Bool);
    AddVariable(AC, "TempVar6", AnimVariable_Bool);
    AddVariable(AC, "TempVar7", AnimVariable_Bool);
    AddVariable(AC, "TempVar8", AnimVariable_Bool);
    AddVariable(AC, "TempVar9", AnimVariable_Bool);
    AddVariable(AC, "TempVar10", AnimVariable_Bool);
    AddVariable(AC, "TempVar11", AnimVariable_Bool);
    AddVariable(AC, "TempVar12", AnimVariable_Bool);
    AddVariable(AC, "TempVar13", AnimVariable_Bool);
    AddVariable(AC, "TempVar14", AnimVariable_Bool);
    AddVariable(AC, "TempVar15", AnimVariable_Bool);
    AddVariable(AC, "TempVar16", AnimVariable_Bool);
    AddVariable(AC, "TempVar17", AnimVariable_Bool);
    AddVariable(AC, "TempVar18", AnimVariable_Bool);
    AddVariable(AC, "TempVar19", AnimVariable_Bool);
    
    anim_variable* FindResVar1 = FindVariable(AC, "TempVar19");
    anim_variable* FindResVar2 = FindVariable(AC, "TempVar122");
#endif
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(AC, "Idle", "Run");
    AddConditionFloat(AC, "VelocityLength",
                      TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(AC);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(AC, "Run", "Idle");
    AddConditionFloat(AC, "VelocityLength",
                      TransitionCondition_LessThan, 0.05f);
    EndTransition(AC);
    
    // NOTE(Dima): Run -> Falling
    BeginTransition(AC, "Run", "Falling");
    AddConditionBool(AC, "IsFalling",
                     TransitionCondition_Equal, true);
    EndTransition(AC);
    
    // NOTE(Dima): Falling -> Idle
    BeginTransition(AC, "Falling", "Idle");
    AddConditionFloat(AC, "VelocityLength",
                      TransitionCondition_LessThan, 0.05f);
    AddConditionBool(AC, "IsFalling",
                     TransitionCondition_Equal, false);
    EndTransition(AC);
    
    // NOTE(Dima): Falling -> Run
    BeginTransition(AC, "Falling", "Run");
    AddConditionFloat(AC, "VelocityLength",
                      TransitionCondition_MoreEqThan, 0.05f);
    AddConditionBool(AC, "IsFalling",
                     TransitionCondition_Equal, false);
    EndTransition(AC);
    
    FinalizeCreation(AC);
    
    return(AC);
}

INTERNAL_FUNCTION void UpdateModel(assets* Assets, 
                                   render_stack* Stack,
                                   model_info* Model, 
                                   v3 Pos, quat Rot, v3 Scale, 
                                   f64 GlobalTime,
                                   f32 DeltaTime,
                                   anim_controller* AC)
{
    asset_id CubeMeshID = GetFirst(Assets, GameAsset_Cube);
    
    anim_calculated_pose CalcPose = UpdateModelAnimation(Assets, Model, AC,
                                                         GlobalTime, DeltaTime, 
                                                         1.0f);
    
    m44 ModelToWorld = ScalingMatrix(Scale) * RotationMatrix(Rot) * TranslationMatrix(Pos);
    
    // NOTE(Dima): Pushing to render
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        // TODO(Dima): Potential bug here
        m44 NodeTran = Node->CalculatedToModel * ModelToWorld;
        //m44 NodeTran = ModelToWorld;
        
        //PushOrLoadMesh(Assets, Stack, CubeMeshID, ScalingMatrix(V3(0.1f)) * NodeTran);
        for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
            asset_id MeshID = Node->MeshIDs[MeshIndex];
            
            mesh_info* Mesh = LoadMesh(Assets, MeshID, ASSET_IMPORT_DEFERRED);
            
            if(Mesh){
                PushMesh(Stack, Mesh, NodeTran, 
                         CalcPose.BoneTransforms, 
                         CalcPose.BoneTransformsCount);
            }
        }
    }
}

struct test_game_mode_state{
    game_camera Camera;
    
    float CameraSpeed;
    float MouseSencitivity;
    
    v3* SphereDistributionsTrig;
    v3* SphereDistributionsFib;
    sphere_distribution SphereDistributionTrig;
    sphere_distribution SphereDistributionFib;
    
#if 0    
    playing_anim Anim0;
    playing_anim Anim1;
    anim_controller AC;
#endif
    
    playing_anim TestAnim;
    anim_controller TestAC;
    
    anim_controller* PlayerAC;
    
    b32 Initialized;
};


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
        
        State->PlayerAC = 0;
        
        State->Initialized = true;
    }
    
    f64 DeltaTime = Game->Render->FrameInfo.dt;
    
    // NOTE(Dima): Processing camera
    game_camera* Camera = &State->Camera;
    
    float CamSpeed = State->CameraSpeed;
    
    if(KeyIsDown(Game->Input, Key_Shift)){
        CamSpeed *= 8.0f;
    }
    
    if(KeyIsDown(Game->Input, Key_Space)){
        CamSpeed *= 10.0f;
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
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    
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
    
    u32 StoolID = GetFirst(Game->Assets, GameAsset_Stool);
    
    
#if 0    
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
                           ASSET_IMPORT_DEFERRED);
        }
    }
#endif
    
    assets* Assets = Game->Assets;
    
    model_info* Model = LoadModel(Game->Assets,
                                  GetFirst(Game->Assets, GameAsset_Man),
                                  ASSET_IMPORT_DEFERRED);
    
    if(Model){
        if(!State->PlayerAC){
            State->PlayerAC = InitPlayerAC(Game->Anim, Game->Assets, Model->NodesCheckSum);
        }
        
        SetStateAnimation(State->PlayerAC, "Idle", Model->AnimationIDs[0]);
        SetStateAnimation(State->PlayerAC, "Run", Model->AnimationIDs[5]);
        SetStateAnimation(State->PlayerAC, "Falling", Model->AnimationIDs[1]);
        
        SetBool(State->PlayerAC, "IsFalling", false);
        
        f32 VelocityLen = 0.0f;
        if(KeyIsDown(Game->Input, Key_Space)){
            VelocityLen = 1.0f;
        }
        
        SetFloat(State->PlayerAC, "VelocityLength", VelocityLen);
        
        UpdateModel(Assets, Stack, Model, 
                    V3(10.0f, 0.0f, 10.0f),
                    QuatI(), 
                    V3(1.0f),
                    Game->Input->Time,
                    Game->Input->DeltaTime,
                    State->PlayerAC);
        
        anim_controller* Control = State->PlayerAC;
        gui_state* Gui = Game->Gui;
        
        v2 P = V2(100.0f, 400.0f);
        GuiBeginLayout(Gui, "Show anim control", GuiLayout_Layout, &P, 0);
        GuiShowInt(Gui, "Playing states count", Control->PlayingStatesCount);
        GuiShowBool(Gui, "In transition", Control->PlayingStatesCount == 2);
        
        for(int PlayingStateIndex = 0;
            PlayingStateIndex < Control->PlayingStatesCount;
            PlayingStateIndex++)
        {
            char BufToShow[64];
            stbsp_sprintf(BufToShow, "%d state name: %s", PlayingStateIndex,
                          Control->PlayingStates[PlayingStateIndex]->Name);
            
            GuiText(Gui, BufToShow);
            
            GuiProgress01(Gui, "Anim phase", 
                          Control->PlayingStates[PlayingStateIndex]->PlayingAnimation.Phase01);
        }
        
        GuiEndLayout(Gui);
    }
    
    
    model_info* TestModel = LoadModel(Game->Assets,
                                      GetFirst(Game->Assets, GameAsset_Test),
                                      ASSET_IMPORT_DEFERRED);
    
    if(TestModel){
        State->TestAnim.AnimationID = TestModel->AnimationIDs[0];
        State->TestAC.PlayingAnimations[0] = &State->TestAnim;
        State->TestAC.PlayingAnimationsCount = 1;
        
        UpdateModel(Assets, Stack, TestModel, 
                    V3(-10.0f, 0.0f, 10.0f),
                    QuatI(), 
                    V3(1.0f),
                    Game->Input->Time,
                    Game->Input->DeltaTime,
                    &State->TestAC);
    }
    
#if 0    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Man),
                    V3(15.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Skyscraper),
                    V3(-50.0f, 0.0f, -50.0f),
                    QuatI(), V3(z10.0f),
                    ASSET_IMPORT_DEFERRED);
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Stool),
                    V3(10.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Bathroom),
                    V3(5.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Heart),
                    V3(0.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_KindPlane),
                    V3(-5.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Podkova),
                    V3(-10.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_RubbishBin),
                    V3(-15.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Snowman),
                    V3(-20.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Toilet),
                    V3(-25.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadModel(Game->Assets, Stack,
                    GetFirst(Game->Assets, GameAsset_Vase),
                    V3(-30.0f, 0.0f, 10.0f),
                    QuatI(), V3(1.0f),
                    ASSET_IMPORT_DEFERRED);
#endif
    
    PushOrLoadBitmap(Game->Assets, Stack,
                     V2(0, 500),
                     V2(100, 100),
                     GetFirst(Game->Assets, GameAsset_Type_Bitmap));
    
#if 0    
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
#endif
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(5.0f, 1.0f + Sin(Game->Input->Time * 2.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(0.0f, 1.0f + Sin(Game->Input->Time * 3.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   CylID,
                   V3(-10.0f, 1.0f, 0.0f), 
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), V3(2.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cylynder),
                   V3(-13.0f, 1.0f, 0.0f),
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), 
                   V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   SphereID,V3(0.0f, 1.0f + Sin(Game->Input->Time * 4.0f), 5.0f), 
                   QuatI(), V3(1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Plane),
                   V3(0.0f, -1.0f, 0.0f), 
                   QuatI(), V3(100.0f),
                   ASSET_IMPORT_DEFERRED);
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
        
        State->Initialized = true;
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

#include "joy_gameplay.cpp"
#include "joy_frogjump_tileedit.cpp"

struct sphere_distribution{
    int Count;
    
    int MaxCount;
    v3 *Samples;
};

enum character_id_type{
    CharacterID_Model,
    
    CharacterID_Failure,
    CharacterID_Fall,
    CharacterID_Idle,
    CharacterID_JumpUp,
    CharacterID_Land,
    CharacterID_Roll,
    CharacterID_Run,
    CharacterID_Sleep,
    CharacterID_Success,
    CharacterID_Talk,
    CharacterID_Walk,
    CharacterID_Die,
    CharacterID_Attack,
    CharacterID_TakeDamage,
    CharacterID_Throw,
    
    CharacterID_Count,
};

struct obj_transform
{
    v3 P;
    v3 S;
    quat R;
};

struct entity_character{
    obj_transform Transform;
    v3 dP;
    
    u32 CharacterIDs[CharacterID_Count];
    
    b32 IsInsect;
    
    model_info* Model;
    animated_component AnimComponent;
};

struct test_game_mode_state{
    game_camera Camera;
    
    f64 GameTime;
    f32 GameDeltaTime;
    f32 WorldSpeedUp;
    
    f32 CameraSpeed;
    f32 MouseSencitivity;
    
    v3* SphereDistributionsTrig;
    v3* SphereDistributionsFib;
    sphere_distribution SphereDistributionTrig;
    sphere_distribution SphereDistributionFib;
    
    model_info* PlayerModel;
    anim_controller* PlayerControl;
    animated_component PlayerAnimComponent;
    
    anim_controller* FriendControl;
    anim_controller* CaterpillarControl;
    
#define TEMP_CHARACTERS_COUNT 200
    entity_character Characters[TEMP_CHARACTERS_COUNT];
    entity_character Caterpillar;
    
    b32 Initialized;
};

struct image_swapper_state{
    float FadeoutTime;
    float ShowTime;
    int ShowIndex;
    int ShowNextIndex;
    float TimeSinceShow;
    float ShowSpeed;
    
    b32 Initialized;
};

// NOTE(Dima): Returns how manu distributions were actually generated
// NOTE(Dima): Should not return more than ToGenerateEstimateCount
#define GEN_SPHERE_DISTRIBUTION_CALLBACK(name) int name(int ToGenerateEstimateCount, v3* Samples)
typedef GEN_SPHERE_DISTRIBUTION_CALLBACK(gen_sphere_distribution_callback);

INTERNAL_FUNCTION GEN_SPHERE_DISTRIBUTION_CALLBACK(GenFibonacciSphereDistributions){
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

INTERNAL_FUNCTION GEN_SPHERE_DISTRIBUTION_CALLBACK(GenTrigonometricSphereDistributions){
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

INTERNAL_FUNCTION void ShowSphereDistributions(asset_system* Assets,
                                               render_state* Render,
                                               sphere_distribution* Distr,
                                               u32 SphereID,
                                               v3 SphereCenter, 
                                               float SphereRad)
{
    PushOrLoadMesh(Assets, Render, 
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
        
        PushOrLoadMesh(Assets, Render, 
                       GetFirst(Assets, AssetEntry_Cube),
                       TargetP, 
                       QuatI(), 
                       V3(0.05f),
                       ASSET_IMPORT_DEFERRED);
    }
    
}

#if 0
// NOTE(Dima): Adding condition variables
AddVariable(Control, "VelocityLength", AnimVariable_Float);
AddVariable(Control, "IsFalling", AnimVariable_Bool);

AddVariable(Control, "VelocityLength", AnimVariable_Float);
#endif

INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitPlayerControl)
{
    anim_controller* Control = CreateAnimControl(Anim);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(Control, "Idle", AnimExitAction_Looping);
    AddAnimState(Control, "Run", AnimExitAction_Looping);
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(Control, "Idle", "Run");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(Control);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(Control, "Run", "Idle");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_LessThan, 0.05f);
    EndTransition(Control);
    
    return(Control);
}

INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitFriendControl){
    anim_controller* Control = CreateAnimControl(Anim);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(Control, "Run", AnimExitAction_Looping);
    AddAnimState(Control, "Falling", AnimExitAction_Looping);
    AddAnimState(Control, "JumpUp", AnimExitAction_Looping);
    AddAnimState(Control, "Land", AnimExitAction_ExitState);
    AddAnimState(Control, "Roll", AnimExitAction_ExitState);
    
    BeginAnimStateQueue(Control, "Idle");
    AddQueueAnimation(Control, "Idle0", AnimExitAction_Next);
    AddQueueAnimation(Control, "Idle1", AnimExitAction_Next);
    EndAnimStateQueue(Control);
    
    f32 SpeedShiftPoint = 0.05f;
    f32 VeryHighFallSpeed = -6.0f;
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(Control, "Idle", "Run", 0.35f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(Control);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(Control, "Run", "Idle");
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, ANIM_ANY_STATE, "JumpUp");
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, ANIM_ANY_STATE, "Falling", 0.5f, false);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, -SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Land", 0.05f, false);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, VeryHighFallSpeed);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Idle", 0.2f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreEqThan, VeryHighFallSpeed);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, 0.5f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Run", 0.2f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreEqThan, VeryHighFallSpeed);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, 0.5f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Land", "Idle", 0.2f, true);
    EndTransition(Control);
    
    anim_state* LandState = FindState(Control, "Land");
    
    BeginTransition(Control, "Land", "Run", 0.3f, true, 0.55f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Idle", 0.25f, true, 0.8f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Run", 0.25f, true, 0.8f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Falling", 0.1f, true, 0.8f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    EndTransition(Control);
    
    return(Control);
}


INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitCaterpillarControl){
    anim_controller* Control = CreateAnimControl(Anim);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(Control, "Run", AnimExitAction_Looping);
    AddAnimState(Control, "Falling", AnimExitAction_Looping);
    AddAnimState(Control, "JumpUp", AnimExitAction_Looping);
    AddAnimState(Control, "Land", AnimExitAction_ExitState);
    AddAnimState(Control, "Roll", AnimExitAction_ExitState);
    AddAnimState(Control, "Die", AnimExitAction_Clamp);
    
    BeginAnimStateQueue(Control, "Idle");
    AddQueueAnimation(Control, "Idle0", AnimExitAction_Random);
    AddQueueAnimation(Control, "Idle1", AnimExitAction_Random);
    AddQueueAnimation(Control, "Idle2", AnimExitAction_Random);
    EndAnimStateQueue(Control);
    
    f32 SpeedShiftPoint = 0.05f;
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(Control, "Idle", "Run", 0.35f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(Control);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(Control, "Run", "Idle");
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, ANIM_ANY_STATE, "JumpUp");
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, ANIM_ANY_STATE, "Falling", 0.5f, false);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, -SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Land", 0.05f, false);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, -6.0f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Idle", 0.2f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreEqThan, -6.0f);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, 0.5f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Falling", "Run", 0.2f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_MoreEqThan, -6.0f);
    AddConditionFloat(Control, "VelocityVertValue", TransitionCondition_LessThan, 0.5f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Land", "Idle", 0.2f, true);
    EndTransition(Control);
    
    BeginTransition(Control, "Land", "Run", 0.3f, true, 0.55f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Idle", 0.25f, true, 0.8f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_LessThan, SpeedShiftPoint);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Run", 0.25f, true, 0.8f);
    AddConditionFloat(Control, "VelocityHorzLen", TransitionCondition_MoreEqThan, SpeedShiftPoint);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    BeginTransition(Control, "Roll", "Falling", 0.1f, true, 0.8f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    EndTransition(Control);
    
    return(Control);
}

INTERNAL_FUNCTION void UpdateModel(asset_system* Assets,
                                   render_state* Render,
                                   model_info* Model, 
                                   obj_transform* Transform, 
                                   v3 Color,
                                   f64 GlobalTime,
                                   f32 DeltaTime,
                                   animated_component* AC)
{
    FUNCTION_TIMING();
    
    asset_id CubeMeshID = GetFirst(Assets, AssetEntry_Cube);
    
    anim_calculated_pose CalcPose = UpdateModelAnimation(Assets, Model, AC,
                                                         GlobalTime, DeltaTime, 
                                                         1.0f);
    
    m44 ModelToWorld = ScalingMatrix(Transform->S) * RotationMatrix(Transform->R) * TranslationMatrix(Transform->P);
    
#if 1
    if(AC)
    {
        v4 ToMove = V4(AC->AdvancedByRootP, 1.0f) * ModelToWorld;
        Transform->P = ToMove.xyz;
        ModelToWorld = ScalingMatrix(Transform->S) * RotationMatrix(Transform->R) * TranslationMatrix(Transform->P);
    }
#endif
    
    // NOTE(Dima): Pushing to render
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        m44 NodeTran = Node->CalculatedToModel * ModelToWorld;
        
        //DEBUGAddAxes(NodeTran);
        
        for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
            asset_id MeshID = Node->MeshIDs[MeshIndex];
            
            mesh_info* Mesh = LoadMesh(Assets, MeshID, ASSET_IMPORT_DEFERRED);
            
            if(Mesh){
                material_info* Material = LoadMaterial(Assets, 
                                                       Model->MaterialIDs[Mesh->MaterialIndexInModel],
                                                       ASSET_IMPORT_DEFERRED);
                
                PushMesh(Render, 
                         &Mesh->Prim, 
                         NodeTran, 
                         &Material->Prim,
                         Color,
                         CalcPose.BoneTransforms, 
                         CalcPose.BoneTransformsCount);
            }
        }
    }
}

INTERNAL_FUNCTION u32 LoadCharacterAssetID(asset_system* Assets, 
                                           u32 GroupID, 
                                           u32 TagCharacterValue)
{
    
    u32 MatchTagTypes[] = {AssetTag_Character};
    asset_tag_value MatchTagValue[] = {TagValue((int)TagCharacterValue)};
    
    u32 ResultID = GetBestByTags(Assets, GroupID, 
                                 MatchTagTypes, MatchTagValue, 1);
    
    return(ResultID);
}

INTERNAL_FUNCTION void InitCharacter(asset_system* Assets, 
                                     entity_character* Character,
                                     u32 TagCharacterValue,
                                     anim_controller* Control,
                                     v3 P, quat R, v3 S, 
                                     b32 IsInsect)
{
    /*
// NOTE(Dima): This check is just to make sure that CharacterID table
has the same count of IDs as GameAssetID table for model and animations
*/
    Assert((CharacterID_Count - CharacterID_Model) == 
           (AssetEntry_Model_TempForCounting - AssetEntry_Model_Character));
    
    // NOTE(Dima): Loading IDs
    u32 *CharIDs = Character->CharacterIDs;
    for(int Index = 0;
        Index < CharacterID_Count;
        Index++)
    {
        CharIDs[Index] = LoadCharacterAssetID(Assets, 
                                              AssetEntry_Model_Character + Index,
                                              TagCharacterValue);
    }
    
    Character->dP = V3(0.0f, 0.0f, 0.0f);
    Character->Transform.P = P;
    Character->Transform.R = R;
    Character->Transform.S = S;
    Character->IsInsect = IsInsect;
    
    model_info* ModelInfo = GET_ASSET_DATA_BY_ID(model_info, AssetType_Model, 
                                                 Assets, CharIDs[CharacterID_Model]);
    
    InitAnimComponent(&Character->AnimComponent, 
                      Control,
                      ModelInfo->NodesCheckSum);
    
    AddVariable(&Character->AnimComponent, "VelocityHorzLen", AnimVariable_Float);
    AddVariable(&Character->AnimComponent, "VelocityVertValue", AnimVariable_Float);
    AddVariable(&Character->AnimComponent, "IsFalling", AnimVariable_Bool);
    
    SetAnimationID(&Character->AnimComponent, "Idle.Idle0", Character->CharacterIDs[CharacterID_Idle]);
    SetAnimationID(&Character->AnimComponent, "Idle.Idle1", Character->CharacterIDs[CharacterID_Idle]);
    SetAnimationID(&Character->AnimComponent, "Run", Character->CharacterIDs[CharacterID_Run]);
    SetAnimationID(&Character->AnimComponent, "JumpUp", Character->CharacterIDs[CharacterID_JumpUp]);
    SetAnimationID(&Character->AnimComponent, "Falling", Character->CharacterIDs[CharacterID_Fall]);
    SetAnimationID(&Character->AnimComponent, "Land", Character->CharacterIDs[CharacterID_Land]);
    SetAnimationID(&Character->AnimComponent, "Roll", Character->CharacterIDs[CharacterID_Roll]);
    
    if(IsInsect){
        SetAnimationID(&Character->AnimComponent, "Idle.Idle2", Character->CharacterIDs[CharacterID_Idle]);
        SetAnimationID(&Character->AnimComponent, "Die", Character->CharacterIDs[CharacterID_Die]);
    }
}

INTERNAL_FUNCTION void UpdateCharacter(asset_system* Assets,
                                       render_state* Render,
                                       input_state* Input,
                                       entity_character* Character,
                                       f64 GlobalTime,
                                       f32 dt)
{
    Character->Model = LoadModel(Assets, 
                                 Character->CharacterIDs[CharacterID_Model], 
                                 ASSET_IMPORT_DEFERRED);
    
    model_info* Model = Character->Model;
    
    f32 PlayingPhase = GetPlayingStatePhase(&Character->AnimComponent);
    b32 IsLandedPlaying = (StateIsPlaying(&Character->AnimComponent, "Land") && 
                           (PlayingPhase < 0.55f));
    b32 IsRollingPlaying = (StateIsPlaying(&Character->AnimComponent, "Roll") && 
                            (PlayingPhase < 0.75f));
    
    b32 CanMove = !IsLandedPlaying && !IsRollingPlaying;
    
    f32 MoveSpeed = 0.0f;
    Character->dP.z = 0.0f;
    if(CanMove && KeyIsDown(Input, Key_Z)){
        MoveSpeed = 1.0f;
        Character->dP.z = 1.0f;
    }
    if(KeyWentDown(Input, Key_Space)){
        Character->dP.y += 5.0f;
    }
    
    animated_component* AC = &Character->AnimComponent;
    
    v3 Acc = V3(0.0f, -9.81f, 0.0f);
    
    Character->Transform.P = Character->Transform.P + Character->dP * dt + Acc * dt * dt * 0.5f;
    
    Character->dP = Character->dP + Acc * dt;
    
    float PrevVelocityY = Character->dP.y;
    
    if(Character->Transform.P.y < 0.0f){
        Character->Transform.P.y = 0.0f;
        Character->dP.y = 0.0f;
    }
    
    b32 IsFalling = Character->Transform.P.y > 0.0001f;
    
    if(KeyWentDown(Input, Key_X) && CanMove && !IsFalling)
    {
        ForceTransitionRequest(&Character->AnimComponent,
                               "Roll", 0.1f, 0.0f);
    }
    
    SetFloat(&Character->AnimComponent, "VelocityHorzLen", MoveSpeed);
    SetFloat(&Character->AnimComponent, "VelocityVertValue", PrevVelocityY);
    SetBool(&Character->AnimComponent, "IsFalling", IsFalling);
    
    if(Model){
        UpdateModel(Assets, Render,
                    Model, 
                    &Character->Transform,
                    V3_One(),
                    GlobalTime, dt,
                    AC);
    }
}

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    FUNCTION_TIMING();
    
    GAME_GET_MODE_STATE(test_game_mode_state, State);
    
    asset_system* Assets = Game->Assets;
    anim_system* Anim = Game->Anim;
    
    render_state* Render = TempState->Render;
    input_state* Input = TempState->Input;
    gui_state* Gui = TempState->Gui;
    
    if(!State->Initialized){
        
        State->GameTime = 0.0;
        State->GameDeltaTime = 0.0001f;
        State->WorldSpeedUp = 1.0f;
        State->Camera = {};
        State->Camera.P = V3(0.0f, 5.0f, 40.0f);
        State->CameraSpeed = 5.0f;
        State->MouseSencitivity = 0.25f;
        
        int SphereDistributionsmaxCount = 1024;
        
        State->SphereDistributionsTrig = PushArray(&Mode->Arena, v3, SphereDistributionsmaxCount);
        State->SphereDistributionsFib = PushArray(&Mode->Arena, v3, SphereDistributionsmaxCount);
        
        State->SphereDistributionTrig = GenerateSphereDistribution(GenTrigonometricSphereDistributions,
                                                                   SphereDistributionsmaxCount,
                                                                   State->SphereDistributionsTrig);
        
        State->SphereDistributionFib = GenerateSphereDistribution(GenFibonacciSphereDistributions,
                                                                  SphereDistributionsmaxCount,
                                                                  State->SphereDistributionsFib);
        
        State->PlayerControl = InitPlayerControl(Anim, Assets, "PlayerControl");
        State->FriendControl = InitFriendControl(Anim, Assets, "FriendControl");
        State->CaterpillarControl = InitCaterpillarControl(Anim, Assets, "CaterpillarControl");
        
#if 0        
        model_info* PlayerInfo = GET_ASSET_DATA_BY_ID(model_info, AssetType_Model, 
                                                      Game->Assets, AssetEntry_Man);
        InitAnimComponent(&State->PlayerAnimComponent,
                          State->PlayerControl,
                          PlayerInfo->NodesCheckSum);
#endif
        
        
        for(int CharIndex = 0;
            CharIndex < TEMP_CHARACTERS_COUNT;
            CharIndex++)
        {
            entity_character* Char = &State->Characters[CharIndex];
            
            
            v3 P = V3((CharIndex % (int)Sqrt(TEMP_CHARACTERS_COUNT)) * 2, 10.0f, 
                      (CharIndex / (int)Sqrt(TEMP_CHARACTERS_COUNT)) * 2);
            
            int CharType = CharIndex % 11;
            
            InitCharacter(Assets, 
                          Char,
                          CharType,
                          State->FriendControl,
                          P,
                          //IdentityQuaternion(),
                          Quat(V3(1.0f, 0.0f, 0.0f), -JOY_PI_OVER_TWO),
                          V3(0.01f),
                          false);
        }
        
        InitCharacter(Assets, &State->Caterpillar,
                      TagCharacter_Caterpillar,
                      State->CaterpillarControl,
                      V3(-10.0f, 10.0f, 10.0f), 
                      Quat(V3(1.0f, 0.0f, 0.0f), -JOY_PI_OVER_TWO),
                      V3(0.01f),
                      true);
        
        State->Initialized = true;
    }
    
    f64 DeltaTime = Render->FrameInfo.dt;
    State->GameDeltaTime = DeltaTime * State->WorldSpeedUp;
    State->GameTime += DeltaTime * State->WorldSpeedUp;
    
    // NOTE(Dima): Processing camera
    game_camera* Camera = &State->Camera;
    
    float CamSpeed = State->CameraSpeed;
    
    if(KeyIsDown(Input, Key_Shift)){
        CamSpeed *= 8.0f;
    }
    
    if(KeyIsDown(Input, Key_Space)){
        CamSpeed *= 10.0f;
    }
    
    float DeltaMouseX = GetMoveAxis(Input, MoveAxis_MouseX) * State->MouseSencitivity;
    float DeltaMouseY = GetMoveAxis(Input, MoveAxis_MouseY) * State->MouseSencitivity;
    
    if(Input->CapturingMouse){
        
        UpdateCameraRotation(Camera, 
                             -DeltaMouseY, 
                             DeltaMouseX,
                             0.0f);
    }
    
    v3 MoveVector = GetMoveVector(Input, -1);
    
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
    
    
    int Width = Render->FrameInfo.InitWidth;
    int Height = Render->FrameInfo.InitHeight;
    
    render_camera_setup CamSetup = DefaultPerspSetup(Render, CameraTransform);
    
    int MainCamSetupIndex = AddCameraSetup(Render, CamSetup);
    
    PushClearColor(Render, V3(0.1f, 0.3f, 0.9f));
    
    char CameraInfo[256];
    stbsp_sprintf(CameraInfo, "P(x%.3f; y%.3f; z%.3f)", 
                  Camera->P.x, 
                  Camera->P.y,
                  Camera->P.z);
    
    ShowText(Gui, CameraInfo);
    
    char MouseInfo[256];
    stbsp_sprintf(MouseInfo,
                  "Delta mouse X: %.2f", 
                  DeltaMouseX);
    ShowText(Gui, MouseInfo);
    stbsp_sprintf(MouseInfo, 
                  "Delta mouse Y: %.2f",
                  DeltaMouseY);
    ShowText(Gui, MouseInfo);
    
    static float FindSphereQuality = 0.5f;
    SliderFloat(Gui, 
                &FindSphereQuality, 
                0.0f, 1.0f,
                "LOD");
    
    SliderFloat(Gui,
                &State->WorldSpeedUp,
                0.1f, 3.0f,
                "World speedup");
    
    SliderFloat(Gui,
                &Render->FogGradient,
                0.1f, 10.0f,
                "Fog gradient");
    
    SliderFloat(Gui,
                &Render->FogDensity,
                0.0f, 0.5f,
                "Fog density");
    
    
    SliderFloat(Gui,
                &Render->FogColor.r,
                0.0f, 1.0f,
                "Fog color R");
    
    SliderFloat(Gui,
                &Render->FogColor.g,
                0.0f, 1.0f,
                "Fog color G");
    
    SliderFloat(Gui,
                &Render->FogColor.b,
                0.0f, 1.0f,
                "Fog color B");
    
    BoolButtonOnOff(Gui, "Fog enabled", &Render->FogEnabled);
    
    SliderFloat(Gui,
                &Render->SSAOKernelRadius,
                0.2f, 1.5f,
                "SSAO Kernel Radius");
    
    SliderInt(Gui,
              &Render->SSAOKernelSampleCount,
              1, SSAO_KERNEL_MAX_SIZE,
              "SSAO Kernel Samples");
    
    SliderFloat(Gui,
                &Render->SSAOContribution,
                0.01f, 1.0f,
                "SSAO Contrib");
    
    SliderFloat(Gui,
                &Render->SSAORangeCheck,
                0.01f, 10.0f,
                "SSAO Range check");
    
    BeginRow(Gui);
    ShowText(Gui, "SSAO Filter");
    ShowText(Gui, (char*)Render->FilterNames[Render->SSAOFilterType]);
    EndRow(Gui);
    
    u32 FindTagTypes[1] = {AssetTag_LOD};
    asset_tag_value FindTagValues[1] = {FindSphereQuality};
    
    u32 SphereID = GetBestByTags(Assets,
                                 AssetEntry_Sphere,
                                 FindTagTypes,
                                 FindTagValues,
                                 1);
    
    u32 CylID = GetBestByTags(Assets,
                              AssetEntry_Cylynder,
                              FindTagTypes,
                              FindTagValues,
                              1);
    
#if 0    
    DEBUGAddLine(V3(-10, 10, 10),
                 V3(10, 10, 10),
                 V3(1.0f, 0.0f, 0.0f),
                 1.0f);
    
    DEBUGAddCross(V3(0, 10, 5.0f), 
                  V3(0.0f, 1.0f, 0.0f),
                  1.0f);
    
    DEBUGAddCircleX(V3(5.0f, 10.0f, 10.0f),
                    V3(1.0f, 0.0f, 0.0f),
                    1.0f);
    
    
    DEBUGAddCircleY(V3(10.0f, 10.0f, 10.0f),
                    V3(1.0f, 0.0f, 0.0f),
                    1.0f);
    
    
    DEBUGAddCircleZ(V3(15.0f, 10.0f, 10.0f),
                    V3(1.0f, 0.0f, 0.0f),
                    1.0f);
    
    DEBUGAddSphere(V3(-15, 10, 10),
                   V3(1.0f, 0.0f, 1.0f),
                   1.0f, 0.0f,
                   false);
    
#if 0    
    int SideSize = 100;
    for(int i = 0; i < SideSize; i++){
        for(int j = 0; j < SideSize; j++){
            DEBUGAddCross(V3(-10.0f, 10.0f, -10.0f) - V3(i, 0, j), 
                          V3(0.0f, 
                             (float)i / (float)SideSize, 
                             (float)j / (float)SideSize),
                          1.0f);
        }
    }
#endif
#endif
    
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
            
            
            PushOrLoadMesh(Assets, Stack, 
                           SphereID,
                           SphereP, QuatI(), V3(1.0f),
                           ASSET_IMPORT_DEFERRED);
        }
    }
#endif
    
    f32 VelocityLen = 0.0f;
    if(KeyIsDown(Input, Key_Space)){
        VelocityLen = 1.0f;
    }
    
#if 0    
    State->PlayerModel = LoadModel(Assets,
                                   GetFirst(Assets, AssetEntry_Man),
                                   ASSET_IMPORT_DEFERRED);
    
    if(State->PlayerModel){
        
        SetStateAnimation(&State->PlayerAnimComponent, "Idle", State->PlayerModel->AnimationIDs[0]);
        SetStateAnimation(&State->PlayerAnimComponent, "Run", State->PlayerModel->AnimationIDs[5]);
        SetStateAnimation(&State->PlayerAnimComponent, "Falling", State->PlayerModel->AnimationIDs[1]);
        
        SetBool(&State->PlayerAnimComponent, "IsFalling", false);
        SetFloat(&State->PlayerAnimComponent, "VelocityLength", VelocityLen);
        
        UpdateModel(Assets, Stack, 
                    State->PlayerModel, 
                    V3(10.0f, 0.0f, 10.0f),
                    QuatI(), 
                    V3(1.0f),
                    State->GameTime,
                    State->GameDeltaTime,
                    &State->PlayerAnimComponent);
    }
#endif
    
#if 1
    int MainQueueIndex = AddRenderQueue(Render);
    
    PushRenderBeginQueue(Render, MainQueueIndex);
    
    for(int CharIndex = 0;
        CharIndex < TEMP_CHARACTERS_COUNT;
        CharIndex++)
    {
        UpdateCharacter(Assets,
                        Render,
                        Input,
                        &State->Characters[CharIndex],
                        State->GameTime,
                        State->GameDeltaTime);
    }
    
    UpdateCharacter(Assets,
                    Render,
                    Input,
                    &State->Caterpillar,
                    State->GameTime,
                    State->GameDeltaTime);
    
    v3 Color00 = V3(1.0f, 0.0f, 0.0f);
    v3 Color01 = V3(0.0f, 1.0f, 0.0f);
    v3 Color10 = V3(0.0f, 0.0f, 1.0f);
    v3 Color11 = V3(1.0f, 0.6f, 0.0f);
    
    int TeapotRowColCount = 20;
    
    for(int RowIndex = 0;
        RowIndex < TeapotRowColCount;
        RowIndex++)
    {
        f32 HorzPercentage = (f32)RowIndex / f32(TeapotRowColCount);
        
        v3 HorzBlend0 = Lerp(Color00, Color01, HorzPercentage);
        v3 HorzBlend1 = Lerp(Color10, Color11, HorzPercentage);
        
        for(int ColIndex = 0;
            ColIndex < TeapotRowColCount;
            ColIndex++)
        {
            f32 VertPercentage = (f32)ColIndex / f32(TeapotRowColCount);
            
            v3 TeapotColor = Lerp(HorzBlend0, HorzBlend1, VertPercentage);
            
            v3 P = V3(-(float)RowIndex, 0.0f, (float)ColIndex);
            
            model_info* Model = LoadModel(Assets, 
                                          GetFirst(Assets, AssetEntry_UtahTeapot), 
                                          ASSET_IMPORT_DEFERRED);
            
            obj_transform Transform;
            Transform.P = P + V3(-10.0f, 0.0f, 10.0f);
            Transform.R = Quat(V3(0.0f, 1.0f, 0.0f), JOY_PI * 0.25f);
            Transform.S = V3(0.01f);
            
            if(Model)
            {
                UpdateModel(Assets, Render,
                            Model, 
                            &Transform,
                            TeapotColor,
                            State->GameTime, State->GameDeltaTime, 0);
            }
        }
    }
    
    PushRenderEndQueue(Render, MainQueueIndex);
    
    PushRenderPass(Render, MainCamSetupIndex, MainQueueIndex);
#endif
    
#if 1
    v3 SphereCenter = V3(0.0f, 7.0f, 0.0f);
    float SphereRad = 2.0f;
    for(int SampleIndex = 0;
        SampleIndex < SSAO_KERNEL_MAX_SIZE;
        SampleIndex++)
    {
        v3 TargetP = SphereCenter + Render->SSAOKernelSamples[SampleIndex] * SphereRad;
        
        PushOrLoadMesh(Assets, Render, 
                       GetFirst(Assets, AssetEntry_Cube),
                       TargetP, 
                       QuatI(), 
                       V3(0.05f),
                       ASSET_IMPORT_DEFERRED);
    }
    
#endif
    
#if 1
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cube),
                   V3(5.0f, 1.0f + Sin(Input->Time * 2.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cube),
                   V3(0.0f, 1.0f + Sin(Input->Time * 3.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Assets, Render, 
                   CylID,
                   V3(-10.0f, 1.0f, 0.0f), 
                   Quat(V3(1.0f, 0.0f, 0.0f), Input->Time), V3(2.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cylynder),
                   V3(-13.0f, 1.0f, 0.0f),
                   Quat(V3(1.0f, 0.0f, 0.0f), Input->Time), 
                   V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Assets, Render, 
                   SphereID,V3(0.0f, 1.0f + Sin(Input->Time * 4.0f), 5.0f), 
                   QuatI(), V3(1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Plane),
                   V3(0.0f, 0.0f, 0.0f), 
                   QuatI(), V3(100.0f),
                   ASSET_IMPORT_DEFERRED);
#endif
    
}

// NOTE(Dima): Changing pictures GAME MODE
GAME_MODE_UPDATE(ChangingPicturesUpdate){
    GAME_GET_MODE_STATE(image_swapper_state, State);
    
    render_state* Render = TempState->Render;
    asset_system* Assets = Game->Assets;
    
    asset_id ArrID = GetFirst(Assets, AssetEntry_FadeoutBmps);
    asset* Asset = GetAssetByID(Assets, ArrID);
    ASSERT(Asset->Type == AssetType_Array);
    
    array_info* Arr = GET_ASSET_PTR_MEMBER(Asset, array_info);
    
    
    render_camera_setup CamSetup = DefaultOrthoSetup(Render, Identity());
    
    PushClearColor(Render, V3(1.0f, 0.5f, 0.0f));
    
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
    
    u32 ToShowID = Arr->FirstID + State->ShowIndex;
    u32 ToShowNextID = Arr->FirstID + State->ShowNextIndex;
    
    bmp_info* ToShow = LoadBmp(Assets, ToShowID, ASSET_IMPORT_DEFERRED);
    bmp_info* ToShowNext = LoadBmp(Assets, ToShowNextID, ASSET_IMPORT_DEFERRED);
    
    int Width = Render->FrameInfo.Width;
    int Height = Render->FrameInfo.Height;
    
    float FadeoutAlpha = Clamp01((State->TimeSinceShow - State->ShowTime) / State->FadeoutTime);
    
    if(ToShow){
        float ToShowH = CalcScreenFitHeight(
                                            ToShow->Prim.Width, 
                                            ToShow->Prim.Height,
                                            Width, Height);
        
        PushBitmap(Render, 
                   &ToShow->Prim, 
                   V2(0.0f, 0.0f), 
                   ToShowH, 
                   V4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    if(ToShowNext){
        float ToShowNextH = CalcScreenFitHeight(
                                                ToShowNext->Prim.Width, 
                                                ToShowNext->Prim.Height,
                                                Width, Height);
        
        PushBitmap(Render, 
                   &ToShowNext->Prim, 
                   V2(0.0f, 0.0f), 
                   ToShowNextH, 
                   V4(1.0f, 1.0f, 1.0f, FadeoutAlpha));
    }
    
    State->TimeSinceShow += Render->FrameInfo.dt * State->ShowSpeed;
    if(State->TimeSinceShow > State->ShowTime + State->FadeoutTime){
        State->ShowIndex = State->ShowNextIndex;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
    }
}


IMPLEMENT_INIT_MODES(){
    DescribeGameMode(Game, "Test", TestUpdate);
    DescribeGameMode(Game, "Changing pictures", ChangingPicturesUpdate);
    DescribeGameMode(Game, "FrogJumpTileEditor", FrogJumpTileEditor);
    
    //SetGameMode(Game, "FrogJumpTileEditor");
    SetGameMode(Game, "Test");
    
    //SetGameMode(Game, "Test");
}

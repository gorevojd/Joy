struct game_camera{
    v3 P;
    v3 dP;
    
    Euler_Angles Angles;
    
    quat Rotation;
};

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

// NOTE(Dima): Updating camera rotation 
void UpdateCameraRotation(game_camera* camera,
                          float dPitch,
                          float dYaw,
                          float dRoll)
{
    float LockEdge = 89.0f * JOY_DEG2RAD;
    
    camera->Angles.Pitch += dPitch * JOY_DEG2RAD;
    camera->Angles.Yaw += dYaw * JOY_DEG2RAD;
    camera->Angles.Roll += dRoll * JOY_DEG2RAD;
    
    camera->Angles.Pitch = Clamp(camera->Angles.Pitch, -LockEdge, LockEdge);
    
    v3 Front;
    Front.x = Sin(camera->Angles.Yaw) * Cos(camera->Angles.Pitch);
    Front.y = Sin(camera->Angles.Pitch);
    Front.z = Cos(camera->Angles.Yaw) * Cos(camera->Angles.Pitch);
    Front = NOZ(Front);
    
    camera->Rotation = QuatLookAt(Front, V3(0.0f, 1.0f, 0.0f));
}

// NOTE(Dima): Look at matrix
m44 GetCameraMatrix(game_camera* camera){
    m44 Result = InverseTranslationMatrix(camera->P) * Transpose(Quat2M44(camera->Rotation));
    
    return(Result);
}

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

INTERNAL_FUNCTION void ShowSphereDistributions(game_state* Game,
                                               render_state* Render,
                                               sphere_distribution* Distr,
                                               u32 SphereID,
                                               v3 SphereCenter, 
                                               float SphereRad)
{
    PushOrLoadMesh(Game->Assets, Render, 
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
        
        PushOrLoadMesh(Game->Assets, Render, 
                       GetFirst(Game->Assets, GameAsset_Cube),
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

INTERNAL_FUNCTION void UpdateModel(assets* Assets,
                                   render_state* Render,
                                   model_info* Model, 
                                   obj_transform* Transform, 
                                   f64 GlobalTime,
                                   f32 DeltaTime,
                                   animated_component* AC)
{
    FUNCTION_TIMING();
    
    asset_id CubeMeshID = GetFirst(Assets, GameAsset_Cube);
    
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
                                                       Model->MaterialIDs[Mesh->MaterialIndex],
                                                       ASSET_IMPORT_DEFERRED);
                
                PushMesh(Render, Mesh, 
                         NodeTran, Material,
                         CalcPose.BoneTransforms, 
                         CalcPose.BoneTransformsCount);
            }
        }
    }
}

INTERNAL_FUNCTION u32 LoadCharacterAssetID(assets* Assets, 
                                           u32 GroupID, 
                                           u32 TagCharacterValue)
{
    
    u32 MatchTagTypes[] = {AssetTag_Character};
    asset_tag_value MatchTagValue[] = {TagValue((int)TagCharacterValue)};
    
    u32 ResultID = GetBestByTags(Assets, GroupID, 
                                 MatchTagTypes, MatchTagValue, 1);
    
    return(ResultID);
}

INTERNAL_FUNCTION void InitCharacter(assets* Assets, 
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
           (GameAsset_Model_TempForCounting - GameAsset_Model_Character));
    
    // NOTE(Dima): Loading IDs
    u32 *CharIDs = Character->CharacterIDs;
    for(int Index = 0;
        Index < CharacterID_Count;
        Index++)
    {
        CharIDs[Index] = LoadCharacterAssetID(Assets, 
                                              GameAsset_Model_Character + Index,
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

INTERNAL_FUNCTION void UpdateCharacter(assets* Assets,
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
                    GlobalTime, dt,
                    AC);
    }
}

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    FUNCTION_TIMING();
    
    GAME_GET_MODE_STATE(test_game_mode_state, State);
    
    render_state* Render = Game->Render;
    assets* Assets = Game->Assets;
    
    if(!State->Initialized){
        
        State->GameTime = 0.0;
        State->GameDeltaTime = 0.0001f;
        State->WorldSpeedUp = 1.0f;
        State->Camera = {};
        State->Camera.P = V3(0.0f, 5.0f, 40.0f);
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
        
        State->PlayerControl = InitPlayerControl(Game->Anim, Game->Assets, "PlayerControl");
        State->FriendControl = InitFriendControl(Game->Anim, Game->Assets, "FriendControl");
        State->CaterpillarControl = InitCaterpillarControl(Game->Anim, Game->Assets, "CaterpillarControl");
        
#if 0        
        model_info* PlayerInfo = GET_ASSET_DATA_BY_ID(model_info, AssetType_Model, 
                                                      Game->Assets, GameAsset_Man);
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
            
            InitCharacter(Game->Assets, 
                          Char,
                          CharType,
                          State->FriendControl,
                          P,
                          //IdentityQuaternion(),
                          Quat(V3(1.0f, 0.0f, 0.0f), -JOY_PI_OVER_TWO),
                          V3(0.01f),
                          false);
        }
        
        InitCharacter(Game->Assets, &State->Caterpillar,
                      TagCharacter_Caterpillar,
                      State->CaterpillarControl,
                      V3(-10.0f, 10.0f, 10.0f), 
                      Quat(V3(1.0f, 0.0f, 0.0f), -JOY_PI_OVER_TWO),
                      V3(0.01f),
                      true);
        
        State->Initialized = true;
    }
    
    f64 DeltaTime = Game->Render->FrameInfo.dt;
    State->GameDeltaTime = DeltaTime * State->WorldSpeedUp;
    State->GameTime += DeltaTime * State->WorldSpeedUp;
    
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
    
    
    int Width = Game->Render->FrameInfo.InitWidth;
    int Height = Game->Render->FrameInfo.InitHeight;
    
    render_camera_setup CamSetup = DefaultPerspSetup(Render, CameraTransform);
    
    int MainCamSetupIndex = AddCameraSetup(Render, CamSetup);
    
    PushClearColor(Render, V3(0.1f, 0.3f, 0.9f));
    
    char CameraInfo[256];
    stbsp_sprintf(CameraInfo, "P(x%.3f; y%.3f; z%.3f)", 
                  Camera->P.x, 
                  Camera->P.y,
                  Camera->P.z);
    
    ShowText(Game->Gui, CameraInfo);
    
    char MouseInfo[256];
    stbsp_sprintf(MouseInfo,
                  "Delta mouse X: %.2f", 
                  DeltaMouseX);
    ShowText(Game->Gui, MouseInfo);
    stbsp_sprintf(MouseInfo, 
                  "Delta mouse Y: %.2f",
                  DeltaMouseY);
    ShowText(Game->Gui, MouseInfo);
    
    static float FindSphereQuality = 0.5f;
    SliderFloat(Game->Gui, 
                &FindSphereQuality, 
                0.0f, 1.0f,
                "LOD");
    
    SliderFloat(Game->Gui,
                &State->WorldSpeedUp,
                0.1f, 3.0f,
                "World speedup");
    
    SliderFloat(Game->Gui,
                &Game->Render->FogGradient,
                0.1f, 10.0f,
                "Fog gradient");
    
    SliderFloat(Game->Gui,
                &Game->Render->FogDensity,
                0.0f, 0.5f,
                "Fog density");
    
    
    SliderFloat(Game->Gui,
                &Game->Render->FogColor.r,
                0.0f, 1.0f,
                "Fog color R");
    
    SliderFloat(Game->Gui,
                &Game->Render->FogColor.g,
                0.0f, 1.0f,
                "Fog color G");
    
    SliderFloat(Game->Gui,
                &Game->Render->FogColor.b,
                0.0f, 1.0f,
                "Fog color B");
    
    BoolButtonOnOff(Game->Gui, "Fog enabled", &Game->Render->FogEnabled);
    
    
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
            
            
            PushOrLoadMesh(Game->Assets, Stack, 
                           SphereID,
                           SphereP, QuatI(), V3(1.0f),
                           ASSET_IMPORT_DEFERRED);
        }
    }
#endif
    
    f32 VelocityLen = 0.0f;
    if(KeyIsDown(Game->Input, Key_Space)){
        VelocityLen = 1.0f;
    }
    
#if 0    
    State->PlayerModel = LoadModel(Game->Assets,
                                   GetFirst(Game->Assets, GameAsset_Man),
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
    
    PushBeginQueue(Render, MainQueueIndex);
    
    for(int CharIndex = 0;
        CharIndex < TEMP_CHARACTERS_COUNT;
        CharIndex++)
    {
        UpdateCharacter(Game->Assets,
                        Render,
                        Game->Input,
                        &State->Characters[CharIndex],
                        State->GameTime,
                        State->GameDeltaTime);
    }
    
    UpdateCharacter(Game->Assets,
                    Render,
                    Game->Input,
                    &State->Caterpillar,
                    State->GameTime,
                    State->GameDeltaTime);
    
    PushEndQueue(Render, MainQueueIndex);
    
    PushRenderPass(Render, MainCamSetupIndex, MainQueueIndex);
#endif
    
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
    
#if 1
    PushOrLoadMesh(Game->Assets, Render, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(5.0f, 1.0f + Sin(Game->Input->Time * 2.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Render, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(0.0f, 1.0f + Sin(Game->Input->Time * 3.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Render, 
                   CylID,
                   V3(-10.0f, 1.0f, 0.0f), 
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), V3(2.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Game->Assets, Render, 
                   GetFirst(Game->Assets, GameAsset_Cylynder),
                   V3(-13.0f, 1.0f, 0.0f),
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), 
                   V3(1.0f), 
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Render, 
                   SphereID,V3(0.0f, 1.0f + Sin(Game->Input->Time * 4.0f), 5.0f), 
                   QuatI(), V3(1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Render, 
                   GetFirst(Game->Assets, GameAsset_Plane),
                   V3(0.0f, 0.0f, 0.0f), 
                   QuatI(), V3(100.0f),
                   ASSET_IMPORT_DEFERRED);
#endif
    
}

// NOTE(Dima): MAIN MENU GAME MODE
GAME_MODE_UPDATE(MainMenuUpdate){
    
}

// NOTE(Dima): Changing pictures GAME MODE
GAME_MODE_UPDATE(ChangingPicturesUpdate){
    GAME_GET_MODE_STATE(image_swapper_state, State);
    
    render_state* Render = Game->Render;
    assets* Assets = Game->Assets;
    
    asset_id ArrID = GetFirst(Game->Assets, GameAsset_FadeoutBmps);
    asset* Asset = GetAssetByID(Game->Assets, ArrID);
    ASSERT(Asset->Type == AssetType_Array);
    
    array_info* Arr = GET_ASSET_PTR_MEMBER(Asset, array_info);
    
    
    render_camera_setup CamSetup = DefaultOrthoSetup(Game->Render, Identity());
    
    //BeginRenderPass(Render, CamSetup);
    
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
    
    bmp_info* ToShow = LoadBmp(Game->Assets, ToShowID, ASSET_IMPORT_DEFERRED);
    bmp_info* ToShowNext = LoadBmp(Game->Assets, ToShowNextID, ASSET_IMPORT_DEFERRED);
    
    int Width = Game->Render->FrameInfo.Width;
    int Height = Game->Render->FrameInfo.Height;
    
    float FadeoutAlpha = Clamp01((State->TimeSinceShow - State->ShowTime) / State->FadeoutTime);
    
    if(ToShow){
        float ToShowH = CalcScreenFitHeight(
                                            ToShow->Width, 
                                            ToShow->Height,
                                            Width, Height);
        
        PushBitmap(Render, 
                   ToShow, 
                   V2(0.0f, 0.0f), 
                   ToShowH, 
                   V4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    if(ToShowNext){
        float ToShowNextH = CalcScreenFitHeight(
                                                ToShowNext->Width, ToShowNext->Height,
                                                Width, Height);
        
        PushBitmap(Render, 
                   ToShowNext, 
                   V2(0.0f, 0.0f), 
                   ToShowNextH, 
                   V4(1.0f, 1.0f, 1.0f, FadeoutAlpha));
    }
    
    State->TimeSinceShow += Game->Render->FrameInfo.dt * State->ShowSpeed;
    if(State->TimeSinceShow > State->ShowTime + State->FadeoutTime){
        State->ShowIndex = State->ShowNextIndex;
        State->ShowNextIndex = (State->ShowIndex + 1) % ToShowCount;
        State->TimeSinceShow = 0.0f;
    }
}

// NOTE(Dima): TITLE GAME MODE
GAME_MODE_UPDATE(TitleUpdate){
    
}
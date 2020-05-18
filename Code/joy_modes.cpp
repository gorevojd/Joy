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
    
    CharacterID_Count,
};

struct entity_character{
    v3 P;
    v3 dP;
    quat R;
    v3 S;
    
    u32 CharacterIDs[CharacterID_Count];
    
    model_info* Model;
    animated_component AnimComponent;
};

struct test_game_mode_state{
    game_camera Camera;
    
    float CameraSpeed;
    float MouseSencitivity;
    
    v3* SphereDistributionsTrig;
    v3* SphereDistributionsFib;
    sphere_distribution SphereDistributionTrig;
    sphere_distribution SphereDistributionFib;
    
    model_info* PlayerModel;
    anim_controller* PlayerControl;
    animated_component PlayerAnimComponent;
    
    anim_controller* FriendControl;
    
    entity_character Characters[100];
    
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

#if 0
// NOTE(Dima): Adding condition variables
AddVariable(Control, "VelocityLength", AnimVariable_Float);
AddVariable(Control, "IsFalling", AnimVariable_Bool);

AddVariable(Control, "VelocityLength", AnimVariable_Float);
#endif

INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitPlayerControl)
{
    anim_controller* Control = CreateAnimControl(Anim, Name);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(Control, AnimState_Animation, "Idle");
    AddAnimState(Control, AnimState_Animation, "Run");
    AddAnimState(Control, AnimState_Animation, "Falling");
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(Control, "Idle", "Run");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(Control);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(Control, "Run", "Idle");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_LessThan, 0.05f);
    EndTransition(Control);
    
    // NOTE(Dima): Run -> Falling
    BeginTransition(Control, "Run", "Falling");
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, true);
    EndTransition(Control);
    
    // NOTE(Dima): Falling -> Idle
    BeginTransition(Control, "Falling", "Idle");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_LessThan, 0.05f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    // NOTE(Dima): Falling -> Run
    BeginTransition(Control, "Falling", "Run");
    AddConditionFloat(Control, "VelocityLength", TransitionCondition_MoreEqThan, 0.05f);
    AddConditionBool(Control, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(Control);
    
    return(Control);
}

INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitFriendControl){
    anim_controller* Control = CreateAnimControl(Anim, Name);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(Control, AnimState_Animation, "Idle");
    AddAnimState(Control, AnimState_Animation, "Run");
    
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

INTERNAL_FUNCTION void UpdateModel(assets* Assets, 
                                   render_stack* Stack,
                                   model_info* Model, 
                                   v3 Pos, quat Rot, v3 Scale, 
                                   f64 GlobalTime,
                                   f32 DeltaTime,
                                   animated_component* AC)
{
    FUNCTION_TIMING();
    
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
        
        m44 NodeTran = Node->CalculatedToModel * ModelToWorld;
        
        //DEBUGAddAxes(NodeTran);
        
        for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
            asset_id MeshID = Node->MeshIDs[MeshIndex];
            
            mesh_info* Mesh = LoadMesh(Assets, MeshID, ASSET_IMPORT_DEFERRED);
            
            if(Mesh){
                material_info* Material = LoadMaterial(Assets, 
                                                       Model->MaterialIDs[Mesh->MaterialIndex],
                                                       ASSET_IMPORT_DEFERRED);
                
                PushMesh(Stack, Mesh, 
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
                                     v3 P, quat R, v3 S)
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
    Character->P = P;
    Character->R = R;
    Character->S = S;
    
    model_info* ModelInfo = GET_ASSET_DATA_BY_ID(model_info, AssetType_Model, 
                                                 Assets, CharIDs[CharacterID_Model]);
    
    InitAnimComponent(&Character->AnimComponent, 
                      Control,
                      ModelInfo->NodesCheckSum);
    
    AddVariable(&Character->AnimComponent, "VelocityLength", AnimVariable_Float);
    AddVariable(&Character->AnimComponent, "IsFalling", AnimVariable_Bool);
    
    SetStateAnimation(&Character->AnimComponent, "Idle", Character->CharacterIDs[CharacterID_Idle]);
    SetStateAnimation(&Character->AnimComponent, "Run", Character->CharacterIDs[CharacterID_Run]);
}

INTERNAL_FUNCTION void UpdateCharacter(assets* Assets,
                                       render_stack* Stack,
                                       input_state* Input,
                                       entity_character* Character,
                                       f64 GlobalTime,
                                       f32 DeltaTime)
{
    Character->Model = LoadModel(Assets, 
                                 Character->CharacterIDs[CharacterID_Model], 
                                 ASSET_IMPORT_DEFERRED);
    
    model_info* Model = Character->Model;
    
    Character->dP = V3(0.0f, 0.0f, 0.0f);
    if(KeyIsDown(Input, Key_Space)){
        Character->dP = V3(0.0f, 0.0f, 1.0f);
    }
    
    SetFloat(&Character->AnimComponent, 
             "VelocityLength", 
             Length(Character->dP));
    
    if(Model){
        UpdateModel(Assets, Stack,
                    Model, Character->P,
                    Character->R, Character->S,
                    GlobalTime, DeltaTime,
                    &Character->AnimComponent);
    }
}

// NOTE(Dima): TEST GAME MODE
GAME_MODE_UPDATE(TestUpdate){
    FUNCTION_TIMING();
    
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
        
        State->PlayerControl = InitPlayerControl(Game->Anim, Game->Assets, "PlayerControl");
        State->FriendControl = InitFriendControl(Game->Anim, Game->Assets, "FriendControl");
        
#if 0        
        model_info* PlayerInfo = GET_ASSET_DATA_BY_ID(model_info, AssetType_Model, 
                                                      Game->Assets, GameAsset_Man);
        InitAnimComponent(&State->PlayerAnimComponent,
                          State->PlayerControl,
                          PlayerInfo->NodesCheckSum);
#endif
        
        
        for(int CharIndex = 0;
            CharIndex < 100;
            CharIndex++)
        {
            entity_character* Char = &State->Characters[CharIndex];
            
            
            v3 P = V3((CharIndex % 10) * 2, 0.0f, 
                      (CharIndex / 10) * 2);
            
            int CharType = (CharIndex * 1234567 - (CharIndex & 3)) % 5;
            
            InitCharacter(Game->Assets, 
                          Char,
                          CharType,
                          State->FriendControl,
                          P, 
                          Quat(V3(1.0f, 0.0f, 0.0f), -JOY_PI_OVER_TWO),
                          V3(0.01f));
        }
        
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
    
    
    int Width = Game->Render->FrameInfo.Width;
    int Height = Game->Render->FrameInfo.Height;
    
    render_camera_setup CamSetup = DefaultPerspSetup(Width, Height, CameraTransform);
    
    render_pass* Pass = BeginRenderPass(Game->Render, CamSetup);
    render_stack* Stack = RenderFindStack(Game->Render, "Main");
    AddStackToRenderPass(Pass, Stack);
    
    PushClearColor(Stack, V3(0.1f, 0.3f, 0.9f));
    
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
    
    assets* Assets = Game->Assets;
    
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
                    Game->Input->Time,
                    Game->Input->DeltaTime,
                    &State->PlayerAnimComponent);
    }
#endif
    
    for(int CharIndex = 0;
        CharIndex < 100;
        CharIndex++)
    {
        UpdateCharacter(Game->Assets,
                        Stack,
                        Game->Input,
                        &State->Characters[CharIndex],
                        Game->Input->Time,
                        Game->Input->DeltaTime);
    }
    
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
    
    asset_id ArrID = GetFirst(Game->Assets, GameAsset_FadeoutBmps);
    asset* Asset = GetAssetByID(Game->Assets, ArrID);
    ASSERT(Asset->Type == AssetType_Array);
    
    array_info* Arr = GET_ASSET_PTR_MEMBER(Asset, array_info);
    
    
    render_camera_setup CamSetup = DefaultOrthoSetup(Game->Render->FrameInfo.Width,
                                                     Game->Render->FrameInfo.Height,
                                                     Identity());
    
    render_pass* Pass = BeginRenderPass(Game->Render, CamSetup);
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
        
        PushBitmap(
                   Stack, 
                   ToShow, 
                   V2(0.0f, 0.0f), 
                   ToShowH, 
                   V4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    
    if(ToShowNext){
        float ToShowNextH = CalcScreenFitHeight(
                                                ToShowNext->Width, ToShowNext->Height,
                                                Width, Height);
        
        PushBitmap(
                   Stack, 
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
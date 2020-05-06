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
                   V3(1.0f, 0.5f, 1.0f),
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
                       V3(0.5f, 1.0f, 1.0f),
                       ASSET_IMPORT_DEFERRED);
    }
    
}


INTERNAL_FUNCTION CREATE_ANIM_CONTROL_FUNC(InitPlayerAC)
{
    anim_controller* AC = CreateAnimControl(Anim, "PlayerAC", NodesCheckSum);
    
    // NOTE(Dima): Adding animation nodes
    AddAnimState(AC, AnimState_Animation, "Idle");
    AddAnimState(AC, AnimState_Animation, "Run");
    AddAnimState(AC, AnimState_Animation, "Falling");
    
    // NOTE(Dima): Adding condition variables
    AddVariable(AC, "VelocityLength", AnimVariable_Float);
    AddVariable(AC, "IsFalling", AnimVariable_Bool);
    
    // NOTE(Dima): Idle -> Run
    BeginTransition(AC, "Idle", "Run");
    AddConditionFloat(AC, "VelocityLength", TransitionCondition_MoreEqThan, 0.05f);
    EndTransition(AC);
    
    // NOTE(Dima): Run -> Idle
    BeginTransition(AC, "Run", "Idle");
    AddConditionFloat(AC, "VelocityLength", TransitionCondition_LessThan, 0.05f);
    EndTransition(AC);
    
    // NOTE(Dima): Run -> Falling
    BeginTransition(AC, "Run", "Falling");
    AddConditionBool(AC, "IsFalling", TransitionCondition_Equal, true);
    EndTransition(AC);
    
    // NOTE(Dima): Falling -> Idle
    BeginTransition(AC, "Falling", "Idle");
    AddConditionFloat(AC, "VelocityLength", TransitionCondition_LessThan, 0.05f);
    AddConditionBool(AC, "IsFalling", TransitionCondition_Equal, false);
    EndTransition(AC);
    
    // NOTE(Dima): Falling -> Run
    BeginTransition(AC, "Falling", "Run");
    AddConditionFloat(AC, "VelocityLength", TransitionCondition_MoreEqThan, 0.05f);
    AddConditionBool(AC, "IsFalling", TransitionCondition_Equal, false);
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
                                   anim_controller* AC,
                                   v3 AlbedoColor)
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
        
        DEBUGAddAxes(NodeTran);
        
        for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
            asset_id MeshID = Node->MeshIDs[MeshIndex];
            
            mesh_info* Mesh = LoadMesh(Assets, MeshID, ASSET_IMPORT_DEFERRED);
            
            if(Mesh){
                PushMesh(Stack, Mesh, NodeTran,
                         AlbedoColor,
                         CalcPose.BoneTransforms, 
                         CalcPose.BoneTransformsCount);
            }
        }
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
    
    
    int Width = Game->Render->FrameInfo.Width;
    int Height = Game->Render->FrameInfo.Height;
    
    render_camera_setup CamSetup = SetupCamera(PerspectiveProjection(Width, Height, 
                                                                     1000.0f, 0.01f),
                                               CameraTransform,
                                               Width, Height, true);
    
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
    
    u32 StoolID = GetFirst(Game->Assets, GameAsset_Stool);
    
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
    
#if 1    
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
                           V3((f32)InLayerIndex / (SphereLayerCount + 1),
                              (f32)LayerIndex / (SphereLayers + 1),
                              0.5f),
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
            
            SetStateAnimation(State->PlayerAC, "Idle", Model->AnimationIDs[0]);
            SetStateAnimation(State->PlayerAC, "Run", Model->AnimationIDs[5]);
            SetStateAnimation(State->PlayerAC, "Falling", Model->AnimationIDs[1]);
        }
        
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
                    State->PlayerAC,
                    V3(1.0f, 0.6f, 0.0f));
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
                    &State->TestAC,
                    V3(0.0f, 0.2f, 1.0f));
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
                   V3(1.0f, 0.0f, 0.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cube),
                   V3(0.0f, 1.0f + Sin(Game->Input->Time * 3.0f) * 0.5f, 0.0f), 
                   QuatI(), V3(1.0f), 
                   V3(1.0f, 1.0f, 0.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   CylID,
                   V3(-10.0f, 1.0f, 0.0f), 
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), V3(2.0f),
                   V3(1.0f, 0.0f, 1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Cylynder),
                   V3(-13.0f, 1.0f, 0.0f),
                   Quat(V3(1.0f, 0.0f, 0.0f), Game->Input->Time), 
                   V3(1.0f), 
                   V3(0.0f, 1.0f, 1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   SphereID,V3(0.0f, 1.0f + Sin(Game->Input->Time * 4.0f), 5.0f), 
                   QuatI(), V3(1.0f),
                   V3(0.5f, 0.0f, 1.0f),
                   ASSET_IMPORT_DEFERRED);
    
    PushOrLoadMesh(Game->Assets, Stack, 
                   GetFirst(Game->Assets, GameAsset_Plane),
                   V3(0.0f, 0.0f, 0.0f), 
                   QuatI(), V3(100.0f),
                   V3(1.0f, 1.0f, 1.0f),
                   ASSET_IMPORT_DEFERRED);
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
    
#if 0    
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
    
    GuiTest(Game->Gui, Game->Render->FrameInfo.dt);
    
    PushBitmap(Stack,
               &Game->Assets->MainLargeAtlas.Bitmap,
               V2(100, 100),
               1000,
               V4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
    
}

// NOTE(Dima): TITLE GAME MODE
GAME_MODE_UPDATE(TitleUpdate){
    
}

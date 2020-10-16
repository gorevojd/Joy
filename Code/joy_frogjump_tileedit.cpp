/*
Threre will be data and multiple arrays of stored data :
-Tiles
-Birds
-Trees maybe
-Drop maybe and other
-Start and finish
-RoadsCoordinates and their speed

Tile should hold information about:
- Type
- If it's walkable or not
*/

enum fj_tile_type
{
    FJ_Tile_Grass,
    FJ_Tile_Road,
    FJ_Tile_Forest,
    FJ_Tile_Water,
    
    FJ_Tile_Count,
};

struct fj_tile
{
    u32 Type;
    b32 IsWalkable;
};

struct frogjump_tile_editor
{
    game_camera Camera;
    
    f64 GameTime;
    f32 GameDeltaTime;
    f32 WorldSpeedUp;
    
    f32 CameraSpeed;
    f32 MouseSencitivity;
    
    int GridWidth;
    int GridHeight;
    int GridMaxDimX;
    int GridMaxDimY;
    fj_tile* TileGrid;
    
    b32 Initialized;
};

GAME_MODE_UPDATE(FrogJumpTileEditor){
    GAME_GET_MODE_STATE(frogjump_tile_editor, State);
    
    render_state* Render = TempState->Render;
    asset_system* Assets = Game->Assets;
    input_state* Input = TempState->Input;
    
    if(!State->Initialized)
    {
        State->GameTime = 0.0;
        State->GameDeltaTime = 0.0001f;
        State->WorldSpeedUp = 1.0f;
        State->Camera = {};
        State->Camera.P = V3(0.0f, 5.0f, 40.0f);
        State->CameraSpeed = 5.0f;
        State->MouseSencitivity = 0.25f;
        
        State->GridWidth = 20;
        State->GridHeight = 20;
        State->GridMaxDimX = 200;
        State->GridMaxDimY = 200;
        State->TileGrid = PushArray(&Mode->Arena, fj_tile, State->GridMaxDimX * State->GridMaxDimY);
        
        State->Initialized = true;
    }
    
    f64 DeltaTime = Render->FrameInfo.dt;
    State->GameDeltaTime = DeltaTime * State->WorldSpeedUp;
    State->GameTime += DeltaTime * State->WorldSpeedUp;
    
    // NOTE(Dima): Calculate camera transform
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
    
    if(Input->CapturingMouse)
    {
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
    
    // NOTE(Dima): Begin render stuff
    render_camera_setup CamSetup = DefaultPerspSetup(Render, CameraTransform);
    int MainCamSetupIndex = AddCameraSetup(Render, CamSetup);
    
    PushClearColor(Render, V3(1.0f, 0.5f, 0.0f));
    
    int MainQueueIndex = AddRenderQueue(Render);
    
    PushRenderBeginQueue(Render, MainQueueIndex);
    
#if 0    
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
#endif
    
    
    v3 Color00 = V3(1.0f, 0.0f, 0.0f);
    v3 Color01 = V3(0.0f, 1.0f, 0.0f);
    v3 Color10 = V3(0.0f, 0.0f, 1.0f);
    v3 Color11 = V3(1.0f, 0.6f, 0.0f);
    
    
    for(int RowIndex = 0;
        RowIndex < State->GridHeight;
        RowIndex++)
    {
        f32 HorzPercentage = (f32)RowIndex / f32(State->GridHeight);
        
        v3 HorzBlend0 = Lerp(Color00, Color01, HorzPercentage);
        v3 HorzBlend1 = Lerp(Color10, Color11, HorzPercentage);
        
        for(int ColIndex = 0;
            ColIndex < State->GridHeight;
            ColIndex++)
        {
            f32 VertPercentage = (f32)ColIndex / f32(State->GridWidth);
            
            v3 TileColor = Lerp(HorzBlend0, HorzBlend1, VertPercentage);
            
            v3 P = V3(-(float)RowIndex, 0.0f, (float)ColIndex);
            
            PushOrLoadMesh(Assets, Render, 
                           GetFirst(Assets, AssetEntry_Plane), P, 
                           QuatI(), V3(1.0f),
                           TileColor,
                           ASSET_IMPORT_DEFERRED);
            
        }
    }
    
    PushRenderEndQueue(Render, MainQueueIndex);
    
    PushRenderPass(Render, MainCamSetupIndex, MainQueueIndex);
}

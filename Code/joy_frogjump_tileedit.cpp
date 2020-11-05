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

enum fj_fill_tile_type
{
    FJ_Fill_Type,
    FJ_Fill_Walkable,
};

struct fj_tile
{
    u32 Type;
    b32 IsWalkable;
};

struct fj_tileed
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

void FJ_FillBox(fj_tileed* Ed, 
                int StartX, int StartY, 
                int Width, int Height,
                u32 FillType, u32 Value)
{
    for(int y = StartY; y < StartY + Height; y++)
    {
        for(int x = StartX; x < StartX + Width; x++)
        {
            fj_tile* Tile = &Ed->TileGrid[y * Ed->GridWidth + x];
            
            switch(FillType)
            {
                case FJ_Fill_Type:
                {
                    Tile->Type = Value;
                }break;
                
                case FJ_Fill_Walkable:
                {
                    Tile->IsWalkable = Value;
                }break;
                
            }
        }
    }
}

void FJ_FillBoundaries(fj_tileed* Ed, u32 TileType)
{
    // NOTE(Dima): Filling with type
    FJ_FillBox(Ed, 0, 0, 
               Ed->GridWidth, 1, 
               FJ_Fill_Type, TileType);
    
    FJ_FillBox(Ed, 0, Ed->GridHeight - 1, 
               Ed->GridWidth, 1, 
               FJ_Fill_Type, TileType);
    
    FJ_FillBox(Ed, 0, 1, 
               1, Ed->GridHeight - 1, 
               FJ_Fill_Type, TileType);
    
    FJ_FillBox(Ed, Ed->GridWidth - 1, 1, 
               1, Ed->GridHeight - 1, 
               FJ_Fill_Type, TileType);
    
    //NOTE(Dima): Filling with non-walkable
    FJ_FillBox(Ed, 0, 0, 
               Ed->GridWidth, 1, 
               FJ_Fill_Walkable, false);
    
    FJ_FillBox(Ed, 0, Ed->GridHeight - 2, 
               Ed->GridWidth, 1, 
               FJ_Fill_Walkable, false);
    
    FJ_FillBox(Ed, 0, 1, 
               1, Ed->GridHeight - 1, 
               FJ_Fill_Walkable, false);
    
    FJ_FillBox(Ed, Ed->GridWidth - 1, 1, 
               1, Ed->GridHeight - 1, 
               FJ_Fill_Walkable, false);
}

void FJ_FillAllWith(fj_tileed* Ed, u32 TileType)
{
    FJ_FillBox(Ed, 0, 0, 
               Ed->GridWidth, Ed->GridHeight, 
               FJ_Fill_Type, TileType);
}

GAME_MODE_UPDATE(FrogJumpTileEditor){
    GAME_GET_MODE_STATE(fj_tileed, State);
    
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
        
        FJ_FillAllWith(State, FJ_Tile_Grass);
        FJ_FillBoundaries(State, FJ_Tile_Forest);
        
        FJ_FillBox(State, 3, 3, 3, 2, FJ_Fill_Type, FJ_Tile_Water);
        FJ_FillBox(State, 13, 9, 3, 4, FJ_Fill_Type, FJ_Tile_Water);
        
        
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
    
    v3 TileColors[FJ_Tile_Count] = {
        V3(0.1f, 0.9f, 0.15f),
        V3(0.8f, 0.06f, 0.06f),
        V3(0.9f, 0.5f, 0.1f),
        V3(0.1f, 0.3f, 0.85f),
    };
    
    for(int y = 0;
        y < State->GridHeight;
        y++)
    {
        for(int x = 0;
            x < State->GridWidth;
            x++)
        {
            fj_tile* Tile = &State->TileGrid[y * State->GridWidth + x];
            
            v3 TileColor = TileColors[Tile->Type];
            v3 P = V3(-(float)x, 0.0f, (float)y);
            
            PushOrLoadMesh(Assets, Render, 
                           GetFirst(Assets, AssetEntry_Plane), P, 
                           QuatI(), V3(1.0f),
                           TileColor,
                           ASSET_IMPORT_DEFERRED);
        }
    }
    
#if 1    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cube), 
                   V3_Up() * 3.0f, 
                   QuatI(), V3(1.0f),
                   V3(0.0f, 1.0f, 0.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cube), 
                   V3_Left() * 3.0f, 
                   QuatI(), V3(1.0f),
                   V3(1.0f, 0.0f, 0.0f),
                   ASSET_IMPORT_DEFERRED);
    
    
    PushOrLoadMesh(Assets, Render, 
                   GetFirst(Assets, AssetEntry_Cube), 
                   V3_Forward() * 3.0f, 
                   QuatI(), V3(1.0f),
                   V3(0.0f, 0.0f, 1.0f),
                   ASSET_IMPORT_DEFERRED);
#endif
    
    
    
    PushRenderEndQueue(Render, MainQueueIndex);
    
    PushRenderPass(Render, MainCamSetupIndex, MainQueueIndex);
}

INTERNAL_FUNCTION render_camera_setup SetupCamera(const m44& Projection, 
                                                  const m44& View, 
                                                  int FramebufferWidth,
                                                  int FramebufferHeight,
                                                  float Far, float Near,
                                                  b32 CalcFrustumPlanes)
{
    render_camera_setup Result = {};
    
    m44 ViewProjection  = View * Projection;
    
    Result.Projection = Projection;
    Result.View = View;
    Result.ViewProjection = ViewProjection;
    
    Result.FramebufferWidth = FramebufferWidth;
    Result.FramebufferHeight = FramebufferHeight;
    
    Result.Far = Far;
    Result.Near = Near;
    
    if(CalcFrustumPlanes){
        v4 *FrustumPlanes = Result.FrustumPlanes;
        
        //NOTE(dima): Left plane
        FrustumPlanes[0].A = ViewProjection.e[3] + ViewProjection.e[0];
        FrustumPlanes[0].B = ViewProjection.e[7] + ViewProjection.e[4];
        FrustumPlanes[0].C = ViewProjection.e[11] + ViewProjection.e[8];
        FrustumPlanes[0].D = ViewProjection.e[15] + ViewProjection.e[12];
        
        //NOTE(dima): Right plane
        FrustumPlanes[1].A = ViewProjection.e[3] - ViewProjection.e[0];
        FrustumPlanes[1].B = ViewProjection.e[7] - ViewProjection.e[4];
        FrustumPlanes[1].C = ViewProjection.e[11] - ViewProjection.e[8];
        FrustumPlanes[1].D = ViewProjection.e[15] - ViewProjection.e[12];
        
        //NOTE(dima): Bottom plane
        FrustumPlanes[2].A = ViewProjection.e[3] + ViewProjection.e[1];
        FrustumPlanes[2].B = ViewProjection.e[7] + ViewProjection.e[5];
        FrustumPlanes[2].C = ViewProjection.e[11] + ViewProjection.e[9];
        FrustumPlanes[2].D = ViewProjection.e[15] + ViewProjection.e[13];
        
        //NOTE(dima): Top plane
        FrustumPlanes[3].A = ViewProjection.e[3] - ViewProjection.e[1];
        FrustumPlanes[3].B = ViewProjection.e[7] - ViewProjection.e[5];
        FrustumPlanes[3].C = ViewProjection.e[11] - ViewProjection.e[9];
        FrustumPlanes[3].D = ViewProjection.e[15] - ViewProjection.e[13];
        
        //NOTE(dima): Near plane
        FrustumPlanes[4].A = ViewProjection.e[3] + ViewProjection.e[2];
        FrustumPlanes[4].B = ViewProjection.e[7] + ViewProjection.e[6];
        FrustumPlanes[4].C = ViewProjection.e[11] + ViewProjection.e[10];
        FrustumPlanes[4].D = ViewProjection.e[15] + ViewProjection.e[14];
        
        //NOTE(dima): Far plane
        FrustumPlanes[5].A = ViewProjection.e[3] - ViewProjection.e[2];
        FrustumPlanes[5].B = ViewProjection.e[7] - ViewProjection.e[6];
        FrustumPlanes[5].C = ViewProjection.e[11] - ViewProjection.e[10];
        FrustumPlanes[5].D = ViewProjection.e[15] - ViewProjection.e[14];
        
        // NOTE(Dima): Normalizing planes
        for (int PlaneIndex = 0;
             PlaneIndex < 6;
             PlaneIndex++)
        {
            FrustumPlanes[PlaneIndex] = NormalizePlane(FrustumPlanes[PlaneIndex]);
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION render_camera_setup DefaultPerspSetup(render_state* Render, 
                                                        const m44& CameraTransform)
{
    render_camera_setup CamSetup = SetupCamera(PerspectiveProjection(Render->InitWindowWidth, 
                                                                     Render->InitWindowHeight, 
                                                                     RENDER_DEFAULT_FAR, 
                                                                     RENDER_DEFAULT_NEAR),
                                               CameraTransform,
                                               Render->InitWindowWidth, 
                                               Render->InitWindowHeight, 
                                               RENDER_DEFAULT_FAR, 
                                               RENDER_DEFAULT_NEAR, 
                                               true);
    
    return(CamSetup);
}

INTERNAL_FUNCTION render_camera_setup DefaultOrthoSetup(render_state* Render, 
                                                        const m44& CameraTransform)
{
    render_camera_setup CamSetup = SetupCamera(OrthographicProjection(Render->InitWindowWidth, 
                                                                      Render->InitWindowHeight),
                                               CameraTransform,
                                               Render->InitWindowWidth, 
                                               Render->InitWindowHeight, 
                                               RENDER_DEFAULT_FAR, 
                                               RENDER_DEFAULT_NEAR, 
                                               true);
    
    return(CamSetup);
}

INTERNAL_FUNCTION int AddCameraSetup(render_state* Render,
                                     render_camera_setup CameraSetup)
{
    Assert(Render->CameraSetupsCount < RENDER_MAX_CAMERA_SETUPS);
    int Result = Render->CameraSetupsCount++;
    
    Render->CameraSetups[Result] = CameraSetup;
    
    return(Result);
}

INTERNAL_FUNCTION int AddRenderQueue(render_state* Render){
    Assert(Render->QueuesCount < RENDER_MAX_QUEUES);
    int Result = Render->QueuesCount++;
    
    return(Result);
}

INTERNAL_FUNCTION void RenderInitGuiGeom(render_state* Render){
    // NOTE(Dima): Init GUI geometry
    int VerticesCount = 50000;
    int IndCount = VerticesCount * 1.5;
    
    Render->GuiGeom.MaxVerticesCount = VerticesCount;
    Render->GuiGeom.MaxIndicesCount = IndCount;
    Render->GuiGeom.MaxTriangleGeomTypesCount = IndCount / 3;
    
    Render->GuiGeom.VerticesCount = 0;
    Render->GuiGeom.IndicesCount = 0;
    Render->GuiGeom.TriangleGeomTypesCount = 0;
    
    Render->GuiGeom.Vertices = PushArray(Render->MemRegion, render_gui_geom_vertex, VerticesCount);
    Render->GuiGeom.Indices = PushArray(Render->MemRegion, u32, IndCount);
    Render->GuiGeom.TriangleGeomTypes = PushArray(
                                                  Render->MemRegion, u8, 
                                                  Render->GuiGeom.MaxTriangleGeomTypesCount);
    
    // NOTE(Dima): Init GUI lines
    int LinesCount = 100000;
    
    Render->GuiGeom.MaxLinePointsCount = LinesCount * 2;
    Render->GuiGeom.MaxLineColorsCount = LinesCount;
    
    Render->GuiGeom.LinePointsCount = 0;
    Render->GuiGeom.LineColorsCount = 0;
    
    Render->GuiGeom.LinePoints = PushArray(Render->MemRegion, v2, 
                                           Render->GuiGeom.MaxLinePointsCount);
    Render->GuiGeom.LineColors = PushArray(Render->MemRegion, v4,
                                           Render->GuiGeom.MaxLineColorsCount);
}

INTERNAL_FUNCTION void RenderInitLinesGeom(render_state* Render){
    Render->LinesGeom.FirstDepth = 0;
    Render->LinesGeom.FirstNoDepth = 0;
    Render->LinesGeom.ChunksAllocated = 0;
}

INTERNAL_FUNCTION void RenderInit(render_state* Render, 
                                  int InitWindowWidth,
                                  int InitWindowHeight,
                                  render_platform_api API){
    InitRandomGeneration(&Render->Random, 12);
    
    Render->FrameInfoIsSet = 0;
    Render->InitWindowWidth = InitWindowWidth;
    Render->InitWindowHeight = InitWindowHeight;
    
    Render->FogEnabled = RENDER_FOG_ENABLED;
    Render->FogDensity = RENDER_FOG_DENSITY;
    Render->FogGradient = RENDER_FOG_GRADIENT;
    Render->FogColor = V3(0.8f, 0.8f, 0.8f);
    
    // NOTE(Dima): Init render API
    Render->PlatformRenderAPI = API;
    
    Render->StackRegion = PushSplit(Render->MemRegion, Megabytes(4));
    
    // NOTE(Dima): Init SSAO kernel
    for(int KernelSampleIndex = 0;
        KernelSampleIndex < SSAO_KERNEL_SIZE;
        KernelSampleIndex++)
    {
        v3 RandomVector = V3(RandomBi(&Render->Random),
                             RandomBi(&Render->Random),
                             RandomUni(&Render->Random));
        
        RandomVector = Normalize(RandomVector);
        
        float Scale = float(KernelSampleIndex) / float(SSAO_KERNEL_SIZE);
        float RandomScale = RandomUni(&Render->Random);
        RandomScale = Lerp(RandomScale, 0.1f, 1.0f);
        
        RandomVector *= RandomScale;
        
        Render->SSAOKernelSamples[KernelSampleIndex] = RandomVector;
    }
    
    // NOTE(Dima): Init SSAO noise texture
    for(int NoiseIndex = 0;
        NoiseIndex < SSAO_NOISE_TEXTURE_SIZE;
        NoiseIndex++)
    {
        v3 Noise = V3(RandomBi(&Render->Random),
                      RandomBi(&Render->Random),
                      0.0f);
        
        Render->SSAONoiseTexture[NoiseIndex] = NOZ(Noise);
    }
    
    RenderInitGuiGeom(Render);
    RenderInitLinesGeom(Render);
    
    Render->PlatformRenderAPI.Init(Render);
}

INTERNAL_FUNCTION void RenderBeginFrame(render_state* Render){
    Render->FrameInfoIsSet = 0;
    Render->FrameInfo = {};
    
    Render->CameraSetupsCount = 0;
    Render->QueuesCount = 0;
    
    // NOTE(Dima): Reset gui
    Render->GuiGeom.VerticesCount = 0;
    Render->GuiGeom.IndicesCount = 0;
    Render->GuiGeom.TriangleGeomTypesCount = 0;
    
    // NOTE(Dima): Reset gui lines
    Render->GuiGeom.LinePointsCount = 0;
    Render->GuiGeom.LineColorsCount = 0;
    
    // NOTE(Dima): Reset lines
    render_lines_chunk* AtChunk = Render->LinesGeom.FirstDepth;
    while(AtChunk){
        AtChunk->Count = 0;
        
        AtChunk = AtChunk->Next;
    }
    
    AtChunk = Render->LinesGeom.FirstNoDepth;
    while(AtChunk){
        AtChunk->Count = 0;
        
        AtChunk = AtChunk->Next;
    }
    
    // NOTE(Dima): Render stack begin frame
    Render->StackRegion.CreationBlock.Used = 0;
    Render->EntryCount = 0;
    
    Render->CurAtlas = 0;
    
    Render->IsSoftwareRenderer = (Render->PlatformRenderAPI.RendererType == Renderer_Software);
}

INTERNAL_FUNCTION void RenderEverything(render_state* Render){
    Render->PlatformRenderAPI.Render(Render);
}

INTERNAL_FUNCTION void RenderEndFrame(render_state* Render){
    
    // NOTE(Dima): Clearing all GUI chunks
    Render->GuiGeom.CurChunkIndex = 0;
}

INTERNAL_FUNCTION void RenderSwapBuffers(render_state* Render){
    Render->PlatformRenderAPI.SwapBuffers(Render);
}

INTERNAL_FUNCTION void RenderFree(render_state* Render){
    Render->PlatformRenderAPI.Free(Render);
}
INTERNAL_FUNCTION render_camera_setup SetupCamera(const m44& Projection, 
                                                  const m44& View, 
                                                  int FramebufferWidth,
                                                  int FramebufferHeight, 
                                                  b32 CalcFrustumPlanes)
{
    render_camera_setup Result = {};
    
    m44 ViewProjection  = View * Projection;
    
    Result.Projection = Projection;
    Result.View = View;
    Result.ViewProjection = ViewProjection;
    
    Result.FramebufferWidth = FramebufferWidth;
    Result.FramebufferHeight = FramebufferHeight;
    
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

INTERNAL_FUNCTION void InitRenderStack(render_stack* Stack, 
                                       render_state* render,
                                       char* Name, 
                                       void* MemoryForStack, 
                                       mi Size)
{
    *Stack = {};
    
    Stack->Render = render;
    Stack->MemRegion = CreateInsideBlock(MemoryForStack, Size);
    CopyStrings(Stack->Name, Name);
    
    Stack->EntryCount = 0;
}

INTERNAL_FUNCTION inline void RenderStackBeginFrame(render_stack* Stack){
    Stack->MemRegion.CreationBlock.Used = 0;
    Stack->EntryCount = 0;
    
    Stack->CurAtlas = 0;
    
    Stack->IsSoftwareRenderer = (Stack->Render->API.RendererType == Renderer_Software);
}

INTERNAL_FUNCTION inline void RenderStackEndFrame(render_stack* Stack){
    
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

INTERNAL_FUNCTION void RenderInit(render_state* Render, render_platform_api API){
    Render->StacksCount = 0;
    
    Render->FrameInfoIsSet = 0;
    
    // NOTE(Dima): Init render API
    Render->API = API;
    
    RenderInitGuiGeom(Render);
    RenderInitLinesGeom(Render);
}

INTERNAL_FUNCTION render_stack* RenderFindStack(render_state* Render, char* Name){
    render_stack* Result = 0;
    
    char NameUpperBuf[256];
    StringToUpper(NameUpperBuf, Name);
    
    for(int i = 0; i < Render->StacksCount; i++){
        if(StringsAreEqual(Render->Stacks[i].Name, NameUpperBuf)){
            Result = &Render->Stacks[i];
            break;
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION render_stack* RenderAddStack(render_state* render, char* Name, mi Size)
{
    render_stack* Result = RenderFindStack(render, Name);
    
    if(!Result){
        char NameUpperBuf[256];
        StringToUpper(NameUpperBuf, Name);
        
        ASSERT(render->StacksCount < ARRAY_COUNT(render->Stacks));
        render_stack* Result = render->Stacks + render->StacksCount++;
        
        void* StackMemory = PushSomeMem(render->MemRegion, Size);
        InitRenderStack(Result, render, NameUpperBuf, StackMemory, Size);
    }
    
    return(Result);
}

/*
 NOTE(Dima): This function tries to find the stack
 in render. If it can not find the stack with the 
 specified name then it creates one.
*/
INTERNAL_FUNCTION render_stack* RenderFindAddStack(render_state* Render, char* Name){
    render_stack* Result = 0;
    
    render_stack* Found = RenderFindStack(Render, Name);
    if(!Found){
        Result = RenderAddStack(Render, Name, RENDER_DEFAULT_STACK_SIZE);
    }
    else{
        Result = Found;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void RenderBeginFrame(render_state* render){
    render->FrameInfoIsSet = 0;
    render->FrameInfo = {};
    
    // NOTE(Dima): Reset gui
    render->GuiGeom.VerticesCount = 0;
    render->GuiGeom.IndicesCount = 0;
    render->GuiGeom.TriangleGeomTypesCount = 0;
    
    // NOTE(Dima): Reset gui lines
    render->GuiGeom.LinePointsCount = 0;
    render->GuiGeom.LineColorsCount = 0;
    
    // NOTE(Dima): Reset lines
    render_lines_chunk* AtChunk = render->LinesGeom.FirstDepth;
    while(AtChunk){
        AtChunk->Count = 0;
        
        AtChunk = AtChunk->Next;
    }
    
    AtChunk = render->LinesGeom.FirstNoDepth;
    while(AtChunk){
        AtChunk->Count = 0;
        
        AtChunk = AtChunk->Next;
    }
    
    // NOTE(Dima): Init render stacks
    for(int StackIndex = 0; 
        StackIndex < render->StacksCount;
        StackIndex++)
    {
        render_stack* ToInitStack = &render->Stacks[StackIndex];
        
        RenderStackBeginFrame(ToInitStack);
    }
}

INTERNAL_FUNCTION void RenderEndFrame(render_state* render){
    
    // NOTE(Dima): Deinit render stacks
    for(int StackIndex = 0; 
        StackIndex < render->StacksCount;
        StackIndex++)
    {
        render_stack* ToInitStack = &render->Stacks[StackIndex];
        
        RenderStackEndFrame(ToInitStack);
    }
    
    for(int PassIndex = 0;
        PassIndex < render->PassCount;
        PassIndex++)
    {
        render_pass* Pass = &render->Passes[PassIndex];
        
        *Pass = {};
    }
    
    // NOTE(Dima): Clearing all GUI chunks
    render->GuiGeom.CurChunkIndex = 0;
    
    render->PassCount = 0;
}

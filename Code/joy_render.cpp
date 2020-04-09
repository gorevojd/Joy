#include "joy_render.h"
#include "joy_software_renderer.h"

void RenderPassSetCamera(render_pass* Pass, m44 Projection, m44 View, 
                         int FramebufferWidth,
                         int FramebufferHeight)
{
    m44 ViewProjection  = View * Projection;
    
    Pass->Projection = Projection;
    Pass->View = View;
    Pass->ViewProjection = ViewProjection;
    
    Pass->FramebufferWidth = FramebufferWidth;
    Pass->FramebufferHeight = FramebufferHeight;
    
    v4 *FrustumPlanes = Pass->FrustumPlanes;
    
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
    
    Stack->IsSoftwareRenderer = (Stack->Render->RendererType == Renderer_Software);
}

INTERNAL_FUNCTION inline void RenderStackEndFrame(render_stack* Stack){
    
}

void RenderInit(render_state* render, render_platform_api API){
    render->StacksCount = 0;
    
    render->FrameInfoIsSet = 0;
    
    
    // NOTE(Dima): Init render API
    render->RendererType = API.RendererType;
    render->SwapBuffers = API.SwapBuffers;
    render->Init = API.Init;
    render->Free = API.Free;
    render->Render = API.Render;
    
    int VerticesCount = 50000;
    int IndCount = VerticesCount * 1.5;
    
    render->GuiGeom.MaxVerticesCount = VerticesCount;
    render->GuiGeom.MaxIndicesCount = IndCount;
    render->GuiGeom.MaxTriangleGeomTypesCount = IndCount / 3;
    
    render->GuiGeom.VerticesCount = 0;
    render->GuiGeom.IndicesCount = 0;
    render->GuiGeom.TriangleGeomTypesCount = 0;
    
    render->GuiGeom.Vertices = PushArray(render->MemRegion, render_gui_geom_vertex, VerticesCount);
    render->GuiGeom.Indices = PushArray(render->MemRegion, u32, IndCount);
    render->GuiGeom.TriangleGeomTypes = PushArray(
        render->MemRegion, u8, 
        render->GuiGeom.MaxTriangleGeomTypesCount);
}

render_stack* RenderAddStack(render_state* render, char* Name, mi Size)
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

render_stack* RenderFindStack(render_state* Render, char* Name){
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

/*
 NOTE(Dima): This function tries to find the stack
 in render. If it can not find the stack with the 
 specified name then it creates one.
*/
render_stack* RenderFindAddStack(render_state* Render, char* Name){
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

void RenderBeginFrame(render_state* render){
    render->FrameInfoIsSet = 0;
    render->FrameInfo = {};
    
    // NOTE(Dima): Reset count
    render->GuiGeom.VerticesCount = 0;
    render->GuiGeom.IndicesCount = 0;
    render->GuiGeom.TriangleGeomTypesCount = 0;
    
    
    // NOTE(Dima): Init render stacks
    for(int StackIndex = 0; 
        StackIndex < render->StacksCount;
        StackIndex++)
    {
        render_stack* ToInitStack = &render->Stacks[StackIndex];
        
        RenderStackBeginFrame(ToInitStack);
    }
}

void RenderEndFrame(render_state* render){
    
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

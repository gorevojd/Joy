#include "joy_render.h"
#include "joy_software_renderer.h"

INTERNAL_FUNCTION void InitRenderStack(render_stack* stack, 
                                       render_state* render,
                                       char* Name, 
                                       void* mem4stack, 
                                       mi size)
{
    *stack = {};
    
    stack->Render = render;
    stack->MemBlock = InitMemoryBlock(mem4stack, size);
    CopyStrings(stack->Name, Name);
    
    stack->EntryCount = 0;
}

INTERNAL_FUNCTION inline void RenderStackBeginFrame(render_stack* stack){
    stack->MemBlock.Used = 0;
    stack->EntryCount = 0;
    
    stack->CurAtlas = 0;
    
    stack->IsSoftwareRenderer = (stack->Render->RendererType == Renderer_Software);
}

INTERNAL_FUNCTION inline void RenderStackEndFrame(render_stack* stack){
    
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
}

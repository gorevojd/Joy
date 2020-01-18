#include "joy_render.h"
#include "joy_software_renderer.h"

INTERNAL_FUNCTION void InitRenderStack(Render_Stack* stack, 
                                       Render_State* render,
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

INTERNAL_FUNCTION inline void RenderStackBeginFrame(Render_Stack* stack){
    stack->MemBlock.Used = 0;
    stack->EntryCount = 0;
    
    stack->CurAtlas = 0;
    
    stack->IsSoftwareRenderer = (stack->Render->RendererType == Renderer_Software);
}

INTERNAL_FUNCTION inline void RenderStackEndFrame(Render_Stack* stack){
    
}

void RenderInit(Render_State* render){
    render->StacksCount = 0;
    
    render->FrameInfoIsSet = 0;
    
    render->RendererType = Renderer_OpenGL;
    
    int VerticesCount = 50000;
    int IndCount = VerticesCount * 1.5;
    
    render->GuiGeom.MaxVerticesCount = VerticesCount;
    render->GuiGeom.MaxIndicesCount = IndCount;
    render->GuiGeom.MaxTriangleGeomTypesCount = IndCount / 3;
    
    render->GuiGeom.VerticesCount = 0;
    render->GuiGeom.IndicesCount = 0;
    render->GuiGeom.TriangleGeomTypesCount = 0;
    
    render->GuiGeom.Vertices = PushArray(render->MemRegion, Render_Gui_Geom_Vertex, VerticesCount);
    render->GuiGeom.Indices = PushArray(render->MemRegion, u32, IndCount);
    render->GuiGeom.TriangleGeomTypes = PushArray(
        render->MemRegion, u8, 
        render->GuiGeom.MaxTriangleGeomTypesCount);
}

Render_Stack* RenderAddStack(Render_State* render, char* Name, mi Size)
{
    ASSERT(render->StacksCount < ARRAY_COUNT(render->Stacks));
    Render_Stack* Result = render->Stacks + render->StacksCount++;
    
    void* StackMemory = PushSomeMem(render->MemRegion, Size);
    
    InitRenderStack(Result, render, Name, StackMemory, Size);
    
    return(Result);
}

void RenderBeginFrame(Render_State* render){
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
        Render_Stack* ToInitStack = &render->Stacks[StackIndex];
        
        RenderStackBeginFrame(ToInitStack);
    }
}

void RenderEndFrame(Render_State* render){
    
    // NOTE(Dima): Deinit render stacks
    for(int StackIndex = 0; 
        StackIndex < render->StacksCount;
        StackIndex++)
    {
        Render_Stack* ToInitStack = &render->Stacks[StackIndex];
        
        RenderStackEndFrame(ToInitStack);
    }
}

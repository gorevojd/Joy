#ifndef JOY_RENDER_H
#define JOY_RENDER_H

#include "joy_render_stack.h"
#include "joy_memory.h"

#define RENDER_DEFAULT_STACK_SIZE Megabytes(1)

struct render_pass{
    render_stack* Stacks[16];
    int StacksCount;
    
    m44 Projection;
    m44 View;
    m44 ViewProjection;
    
    int FramebufferWidth;
    int FramebufferHeight;
    
    v4 FrustumPlanes[6];
};

struct render_gui_geom_vertex{
    float Data[8];
};

enum render_gui_geom_type{
    RenderGuiGeom_Rect,
    RenderGuiGeom_Bmp,
};

#define DEFERRED_GUI_GEOMETRY_RENDERING 1

#define MAX_CLIP_RECT_STACK_DEPTH 32
#define MAX_GUI_CHUNKS 1024

struct render_gui_chunk{
    int BaseVertex;
    int IndicesCount;
    int StartIndicesCount;
    
    rc2 ClipRect;
};

struct render_gui_geom{
    render_gui_geom_vertex* Vertices;
    u32* Indices;
    u8* TriangleGeomTypes;
    int TriangleGeomTypesCount;
    int VerticesCount;
    int IndicesCount;
    
    int MaxVerticesCount;
    int MaxIndicesCount;
    int MaxTriangleGeomTypesCount;
    
    rc2 ClipRectStack[MAX_CLIP_RECT_STACK_DEPTH];
    int ClipRectStackIndex;
    
    render_gui_chunk Chunks[MAX_GUI_CHUNKS];
    int CurChunkIndex;
};

struct render_frame_info{
    float dt;
    int Width;
    int Height;
    
    u32 RendererType;
    
    bmp_info* SoftwareBuffer;
};

#define RENDER_PLATFORM_SWAPBUFFERS(name) void name()
typedef RENDER_PLATFORM_SWAPBUFFERS(render_platform_swapbuffers);

#define RENDER_PLATFORM_INIT(name) void name(struct assets* Assets)
typedef RENDER_PLATFORM_INIT(render_platform_init);

#define RENDER_PLATFORM_FREE(name) void name()
typedef RENDER_PLATFORM_FREE(render_platform_free);

#define RENDER_PLATFORM_RENDER(name) void name()
typedef RENDER_PLATFORM_RENDER(render_platform_render);


enum Renderer_Type{
    Renderer_None,
    
    Renderer_OpenGL,
    Renderer_Software,
    Renderer_DirectX,
    
    Renderer_Count,
};

struct render_platform_api{
    u32 RendererType;
    
    render_platform_swapbuffers* SwapBuffers;
    render_platform_init* Init;
    render_platform_free* Free;
    render_platform_render* Render;
};

struct render_state{
    mem_region* MemRegion;
    
    render_platform_swapbuffers* SwapBuffers;
    render_platform_init* Init;
    render_platform_free* Free;
    render_platform_render* Render;
    
    render_stack Stacks[16];
    int StacksCount;
    
    render_pass Passes[16];
    int PassCount;
    
    b32 FrameInfoIsSet;
    render_frame_info FrameInfo;
    
    u32 RendererType;
    
    render_gui_geom GuiGeom;
};

inline render_pass* BeginRenderPass(render_state* Render){
    ASSERT(Render->PassCount < ARRAY_COUNT(Render->Passes));
    
    render_pass* Result = &Render->Passes[Render->PassCount++];
    
    return(Result);
}

inline void AddStackToRenderPass(render_pass* Pass, render_stack* Stack){
    ASSERT(Pass->StacksCount < ARRAY_COUNT(Pass->Stacks));
    
    Pass->Stacks[Pass->StacksCount++] = Stack;
}

inline render_gui_geom* GetGuiGeom(render_stack* Stack){
    render_gui_geom* Result = &Stack->Render->GuiGeom;
    
    return(Result);
}

inline void PushGuiGeom_Internal(render_gui_geom* Geom, 
                                 render_gui_geom_vertex arr[4], 
                                 u8 GeomType)
{
    
    u32 VAt = Geom->VerticesCount;
    Geom->Vertices[Geom->VerticesCount++] = arr[0];
    Geom->Vertices[Geom->VerticesCount++] = arr[1];
    Geom->Vertices[Geom->VerticesCount++] = arr[2];
    Geom->Vertices[Geom->VerticesCount++] = arr[3];
    
    u32 At = Geom->IndicesCount;
    Geom->Indices[At + 0] = VAt + 0;
    Geom->Indices[At + 1] = VAt + 1;
    Geom->Indices[At + 2] = VAt + 2;
    Geom->Indices[At + 3] = VAt + 0;
    Geom->Indices[At + 4] = VAt + 2;
    Geom->Indices[At + 5] = VAt + 3;
    Geom->IndicesCount += 6;
    
    u32 GeomTypeAt = Geom->TriangleGeomTypesCount;
    Geom->TriangleGeomTypes[GeomTypeAt + 0] = GeomType;
    Geom->TriangleGeomTypes[GeomTypeAt + 1] = GeomType;
    Geom->TriangleGeomTypesCount += 2;
}

inline void PushGuiGeom_InAtlasBmp(render_gui_geom* Geom, 
                                   v2 P, v2 Dim, 
                                   v2 MinUV, v2 MaxUV, 
                                   v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    v2 Min = P;
    v2 Max = P + Dim;
    
    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;
    
    render_gui_geom_vertex RectArr[] = {
        {Min.x, Max.y, MinUV.x, MaxUV.y, r, g, b, a },
        {Max.x, Max.y, MaxUV.x, MaxUV.y, r, g, b, a },
        {Max.x, Min.y, MaxUV.x, MinUV.y, r, g, b, a },
        {Min.x, Min.y, MinUV.x, MinUV.y, r, g, b, a },
    };
    
    PushGuiGeom_Internal(Geom, RectArr, RenderGuiGeom_Bmp);
}

inline void PushGuiGeom_InAtlasBmp(render_gui_geom* Geom, 
                                   v2 P, v2 Dim, 
                                   In_Atlas_Bitmap bmp, 
                                   v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    PushGuiGeom_InAtlasBmp(Geom, P, Dim, bmp.MinUV, bmp.MaxUV, Color);
}

inline void PushGuiGeom_Rect(render_gui_geom* Geom, 
                             rc2 Rect, 
                             v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Rect = RectNormalizeSubpixel(Rect);
    
    v2 Min = Rect.min;
    v2 Max = Rect.max;
    
    float r = Color.r;
    float g = Color.g;
    float b = Color.b;
    float a = Color.a;
    
    render_gui_geom_vertex RectArr[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r, g, b, a},
        {Max.x, Max.y, 1.0f, 1.0f, r, g, b, a},
        {Max.x, Min.y, 1.0f, 0.0f, r, g, b, a},
        {Min.x, Min.y, 0.0f, 0.0f, r, g, b, a},
    };
    
    PushGuiGeom_Internal(Geom, RectArr, RenderGuiGeom_Rect);
}

inline void PushGuiGeom_Rect(render_gui_geom* Geom, 
                             v2 p, v2 dim,
                             v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 Rect = RcMinDim(p, dim);
    
    PushGuiGeom_Rect(Geom, Rect, multColor);
}

inline void PushGuiGeom_Gradient(render_gui_geom* Geom, 
                                 rc2 rc, 
                                 v3 color1, 
                                 v3 color2, 
                                 u32 gradType)
{
    v2 Min = rc.min;
    v2 Max = rc.max;
    
    float r1 = color1.r;
    float g1 = color1.g;
    float b1 = color1.b;
    
    float r2 = color2.r;
    float g2 = color2.g;
    float b2 = color2.b;
    
    render_gui_geom_vertex RectArr1[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r1, g1, b1, 1.0f},
        {Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Min.y, 1.0f, 0.0f, r2, g2, b2, 1.0f},
        {Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f},
    };
    
    render_gui_geom_vertex RectArr2[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Min.y, 1.0f, 0.0f, r1, g1, b1, 1.0f},
        {Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f},
    };
    
    render_gui_geom_vertex *ArrToUse = 0;
    if(gradType == RenderEntryGradient_Horizontal){
        ArrToUse = RectArr1;
    }
    else if(gradType == RenderEntryGradient_Vertical){
        ArrToUse = RectArr2;
    }
    else{
        INVALID_CODE_PATH;
    }
    
    PushGuiGeom_Internal(Geom, ArrToUse, RenderGuiGeom_Rect);
}

inline void PushGuiGeom_Gradient(render_gui_geom* Geom, 
                                 rc2 rc, 
                                 v4 color1, 
                                 v4 color2, 
                                 u32 gradType)
{
    PushGuiGeom_Gradient(Geom, rc,
                         color1.rgb, color2.rgb,
                         gradType);
}


inline void PushGuiGeom_RectOutline(render_gui_geom* Geom, 
                                    v2 p, v2 dim, 
                                    int pixelWidth, v4 multColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) 
{
    PushGuiGeom_Rect(Geom, 
                     V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(Geom, 
                     V2(p.x - pixelWidth, p.y), 
                     V2(pixelWidth, dim.y + pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(Geom, 
                     V2(p.x, p.y + dim.y), 
                     V2(dim.x + pixelWidth, pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(Geom, 
                     V2(p.x + dim.x, p.y), 
                     V2(pixelWidth, dim.y), 
                     multColor);
}


inline void PushGuiGeom_RectOutline(render_gui_geom* Geom, rc2 rect, 
                                    int pixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) 
{
    v2 p = rect.min;
    v2 dim = GetRectDim(rect);
    
    PushGuiGeom_RectOutline(Geom, p, dim, pixelWidth, Color);
}

inline void PushGuiGeom_InnerOutline(render_gui_geom* Geom, rc2 rect, int pixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
    v2 p = rect.min + V2(pixelWidth, pixelWidth);
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushGuiGeom_RectOutline(Geom, p, dim, pixelWidth, Color);
}


inline void PushClearColor(render_stack* Stack, v3 color){
    render_entry_clear_color* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_ClearColor, render_entry_clear_color);;
    
    entry->clearColor01 = color;
}

inline void PushBitmap(render_stack* Stack, bmp_info* bitmap, v2 p, float height, v4 multColor){
    render_entry_bitmap* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Bitmap, render_entry_bitmap);
    
    entry->Bitmap = bitmap;
    entry->P = p;
    entry->PixelHeight = height;
    entry->ModulationColor01 = multColor;
}

inline void PushGradient(render_stack* Stack, 
                         rc2 rc, 
                         v3 color1, 
                         v3 color2, 
                         u32 gradType)
{
    if(Stack->IsSoftwareRenderer){
        render_entry_gradient* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Gradient, render_entry_gradient);
        
        entry->rc = rc;
        entry->color1 = color1;
        entry->color2 = color2;
        entry->gradType = gradType;
    }
    else{
        PushGuiGeom_Gradient(GetGuiGeom(Stack), rc, color1, color2, gradType);
    }
}

inline void PushGradient(render_stack* Stack, 
                         rc2 rc, 
                         v4 color1, 
                         v4 color2, 
                         u32 gradType)
{
    PushGradient(Stack, rc, color1.rgb, color2.rgb, gradType);
}

inline void PushRect(
render_stack* Stack, 
rc2 rect, 
v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(Stack->IsSoftwareRenderer){
        render_entry_rect* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Rect, render_entry_rect);
        
        rect = RectNormalizeSubpixel(rect);
        
        entry->p = rect.min;
        entry->dim = GetRectDim(rect);
        entry->modulationColor01 = multColor;
    }
    else{
        PushGuiGeom_Rect(GetGuiGeom(Stack), rect, multColor);
    }
}

inline void PushRect(render_stack* Stack, 
                     v2 p, v2 dim,
                     v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 rect = RcMinDim(p, dim);
    
    PushRect(Stack, rect, multColor);
}

inline void PushRectOutline(render_stack* Stack, v2 p, v2 dim, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    if(Stack->IsSoftwareRenderer){
        v2 widthQuad = V2(pixelWidth, pixelWidth);
        PushRect(Stack, V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), multColor);
        PushRect(Stack, V2(p.x - pixelWidth, p.y), V2(pixelWidth, dim.y + pixelWidth), multColor);
        PushRect(Stack, V2(p.x, p.y + dim.y), V2(dim.x + pixelWidth, pixelWidth), multColor);
        PushRect(Stack, V2(p.x + dim.x, p.y), V2(pixelWidth, dim.y), multColor);
    }
    else{
        PushGuiGeom_RectOutline(GetGuiGeom(Stack), p, dim, pixelWidth, multColor);
    }
}

inline void PushRectOutline(render_stack* Stack, rc2 rect, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.min;
    v2 dim = GetRectDim(rect);
    
    PushRectOutline(Stack, p, dim, pixelWidth, multColor);
}

inline void PushRectInnerOutline(render_stack* Stack, rc2 rect, int pixelWidth, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.min + V2(pixelWidth, pixelWidth);;
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushRectOutline(Stack, p, dim, pixelWidth, color);
}

inline void PushGlyph(render_stack* Stack, 
                      v2 P, v2 Dim, 
                      bmp_info* Bitmap, 
                      v2 MinUV, v2 MaxUV,
                      v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(Stack->IsSoftwareRenderer){
        render_entry_bitmap* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Bitmap, render_entry_bitmap);
        
        entry->P = P;
        entry->PixelHeight = Dim.y;
        entry->ModulationColor01 = ModColor;
        entry->Bitmap = Bitmap;
    }
    else{
        PushGuiGeom_InAtlasBmp(GetGuiGeom(Stack), P, Dim, MinUV, MaxUV, ModColor);
    }
}

inline void PushMesh(render_stack* Stack,
                     mesh_info* Mesh,
                     v3 P,
                     quat R,
                     v3 S)
{
    render_entry_mesh* entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Mesh, render_entry_mesh);
    
    entry->Mesh = Mesh;
    entry->Transform = ScalingMatrix(S) * RotationMatrix(R) * TranslationMatrix(P);
}

inline void PushGuiChunk(render_stack* Stack, int ChunkIndex){
    render_entry_gui_chunk* Entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_GuiChunk, render_entry_gui_chunk);
    
    Entry->ChunkIndex = ChunkIndex;
}

inline void BeginGuiChunk(render_stack* Stack, rc2 ClipRect){
    render_gui_geom* Geom = &Stack->Render->GuiGeom;
    
    int CurIndex = Geom->CurChunkIndex;
    render_gui_chunk* CurChunk = &Geom->Chunks[CurIndex];
    
    CurChunk->IndicesCount = Geom->IndicesCount - CurChunk->StartIndicesCount;
    if(CurChunk->IndicesCount > 0){
        PushGuiChunk(Stack, CurIndex);
    }
    
    // NOTE(Dima): Advancing current chunk index
    ASSERT(CurIndex + 1 < MAX_GUI_CHUNKS);
    Geom->CurChunkIndex = CurIndex + 1;
    
    // NOTE(Dima): Setting new chunk
    render_gui_chunk* NewChunk = &Geom->Chunks[Geom->CurChunkIndex];
    NewChunk->IndicesCount = 0;
    NewChunk->BaseVertex = Geom->VerticesCount;
    NewChunk->ClipRect = ClipRect;
    NewChunk->StartIndicesCount = Geom->IndicesCount;
    
    // NOTE(Dima): Adding new clip rect to stack
    ++Geom->ClipRectStackIndex;
    ASSERT(Geom->ClipRectStackIndex < MAX_CLIP_RECT_STACK_DEPTH);
    Geom->ClipRectStack[Geom->ClipRectStackIndex] = ClipRect;
}

inline void EndGuiChunk(render_stack* Stack){
    render_gui_geom* Geom = &Stack->Render->GuiGeom;
    
    int CurIndex = Geom->CurChunkIndex;
    render_gui_chunk* CurChunk = &Geom->Chunks[CurIndex];
    
    CurChunk->IndicesCount = Geom->IndicesCount - CurChunk->StartIndicesCount;
    if(CurChunk->IndicesCount > 0){
        PushGuiChunk(Stack, CurIndex);
    }
    
    // NOTE(Dima): Popping last value from ClipRect's stack
    ASSERT(Geom->ClipRectStackIndex > 0);
    --Geom->ClipRectStackIndex;
    
    // NOTE(Dima): Advancing current chunk index
    ASSERT(CurIndex + 1 < MAX_GUI_CHUNKS);
    Geom->CurChunkIndex = CurIndex + 1;
    
    // NOTE(Dima): Setting new chunk
    render_gui_chunk* NewChunk = &Geom->Chunks[Geom->CurChunkIndex];
    NewChunk->IndicesCount = 0;
    NewChunk->BaseVertex = Geom->VerticesCount;
    NewChunk->ClipRect = Geom->ClipRectStack[Geom->ClipRectStackIndex];
    NewChunk->StartIndicesCount = Geom->IndicesCount;
}

// NOTE(Dima): RENDERER API
inline void RenderSetFrameInfo(render_state* Render, render_frame_info FrameInfo){
    Render->FrameInfo = FrameInfo;
    Render->FrameInfoIsSet = 1;
    
    Render->RendererType = FrameInfo.RendererType;
}

void RenderInit(render_state* Render, 
                render_platform_api API);
render_stack* RenderAddStack(render_state* Render, 
                             char* Name, 
                             mi Size = RENDER_DEFAULT_STACK_SIZE);
render_stack* RenderFindStack(render_state* Render, char* Name);
render_stack* RenderFindAddStack(render_state* Render, char* Name);

void RenderBeginFrame(render_state* Render);
void RenderEndFrame(render_state* Render);

void RenderPassSetCamera(render_pass* Pass, m44 Projection, m44 View, 
                         int FramebufferWidth,
                         int FramebufferHeight);

#endif
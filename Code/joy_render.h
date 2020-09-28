#ifndef JOY_RENDER_H
#define JOY_RENDER_H

#define RENDER_DEFAULT_STACK_SIZE Megabytes(5)
#define RENDER_DEFAULT_FAR 250.0f
#define RENDER_DEFAULT_NEAR 1.0f
#define RENDER_FOG_DENSITY 0.035f
#define RENDER_FOG_GRADIENT 3.0f
#define RENDER_FOG_ENABLED false

#define SSAO_KERNEL_MAX_SIZE 256
#define SSAO_DEFAULT_SAMPLES 64
#define SSAO_DEFAULT_CONTRIBUTION 0.6f
#define SSAO_DEFAULT_RADIUS 0.6f
#define SSAO_DEFAULT_RANGE_CHECK 1.0f
#define SSAO_NOISE_TEXTURE_SIZE 16

#define RENDER_MAX_CAMERA_SETUPS 256
#define RENDER_MAX_QUEUES 256
#define RENDER_DEFAULT_QUEUE_INDEX -1

enum render_entry_type{
    RenderEntry_ClearColor,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
    RenderEntry_Mesh,
    RenderEntry_Glyph,
    RenderEntry_Gradient,
    RenderEntry_GuiGeometryChunk,
    
    RenderEntry_RenderPass,
    
    RenderEntry_BeginQueue,
    RenderEntry_EndQueue,
};

enum render_show_buffer_type{
    RenderShowBuffer_Main = 0,
    
    RenderShowBuffer_Albedo = 1,
    RenderShowBuffer_Specular = 2,
    RenderShowBuffer_Depth = 3,
    RenderShowBuffer_Normal = 4,
    RenderShowBuffer_Metal = 5,
    RenderShowBuffer_Roughness = 6,
    RenderShowBuffer_SSAO = 7,
    RenderShowBuffer_SSAOBlur = 8,
    
    RenderShowBuffer_Count,
};

enum render_filter_type
{
    RenderFilter_GaussianBlur5x5 = 0,
    RenderFilter_GaussianBlur3x3 = 1,
    RenderFilter_BoxBlur5x5 = 2,
    RenderFilter_BoxBlur3x3 = 3,
    
    RenderFilter_Count,
};

enum render_pass_type{
    RenderPass_Main,
    RenderPass_RenderPassEntry,
};

struct render_framebuffer{
    int Width;
    int Height;
};

struct render_camera_setup{
    m44 Projection;
    m44 View;
    m44 ViewProjection;
    
    float Far;
    float Near;
    float FOVRadians;
    
    int FramebufferWidth;
    int FramebufferHeight;
    
    float AspectRatio;
    
    v4 FrustumPlanes[6];
};

struct render_queue{
    void* FirstEntry;
    void* OnePastLastEntry;
    
    b32 Beginned;
    int QueuesCountAtBeginCall;
};

struct render_entry_header{
    u32 Type;
    u32 DataSize;
};

#define RENDER_GET_ENTRY(type) type* Entry = (type*)At

#define RENDER_ENTRY_MEMORY_ALIGN 4
#pragma pack(push, RENDER_ENTRY_MEMORY_ALIGN)
struct render_entry_clear_color{
    v3 clearColor01;
};

struct render_entry_bitmap{
    render_primitive_bitmap* Bitmap;
    v2 P;
    float PixelHeight;
    v4 ModulationColor01;
};

enum RenderEntryGradientType{
    RenderEntryGradient_Horizontal,
    RenderEntryGradient_Vertical,
};

struct render_entry_gradient{
    rc2 rc;
    v3 color1;
    v3 color2;
    u32 gradType;
};

struct render_entry_rect{
    v4 modulationColor01;
    v2 p;
    v2 dim;
};

struct render_entry_glyph{
    v2 P;
    v2 Dim;
    
    v2 MinUV;
    v2 MaxUV;
    
    bmp_info* Bitmap;
    
    v4 ModColor;
};

struct render_entry_in_atlas_bmp{
    v2 P;
    v2 Dim;
    
    v2 MinUV;
    v2 MaxUV;
    
    v4 ModulationColor;
};

struct render_entry_mesh{
    render_primitive_mesh* Mesh;
    render_primitive_material* Material;
    
    m44 Transform;
    
    m44* BoneTransforms;
    int BoneCount;
    
    v3 ModColor;
};

struct render_entry_gui_chunk{
    int ChunkIndex;
};

struct render_entry_renderqueue{
    int QueueIndex;
    int QueuesBeginnedAtBeginCall;
};

struct render_entry_renderpass{
    int CameraSetupIndex;
    int QueueIndex;
};

#pragma pack(pop)

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
    
    int LinePointsCount;
    int LinePointsBase;
    
    rc2 ClipRect;
};

struct render_gui_geom{
    // NOTE(Dima): GUI geometry
    render_gui_geom_vertex* Vertices;
    u32* Indices;
    u8* TriangleGeomTypes;
    int TriangleGeomTypesCount;
    int VerticesCount;
    int IndicesCount;
    
    int MaxVerticesCount;
    int MaxIndicesCount;
    int MaxTriangleGeomTypesCount;
    
    v2* LinePoints; // NOTE(Dima): Per-point
    v4* LineColors; // NOTE(Dima): Per-line
    int LinePointsCount;
    int LineColorsCount;
    
    int MaxLinePointsCount;
    int MaxLineColorsCount;
    
    render_gui_chunk Chunks[MAX_GUI_CHUNKS];
    int CurChunkIndex;
    
    rc2 ClipRectStack[MAX_CLIP_RECT_STACK_DEPTH];
    int ClipRectStackIndex;
    
};

struct render_line_primitive{
    v3 Start;
    v3 End;
};

struct render_lines_chunk{
    b32 DepthEnabled;
    
    render_line_primitive* Lines;
    v3* Colors;
    
    int Count;
    int MaxCount;
    
    render_lines_chunk* Next;
};

struct render_lines_geom{
    render_lines_chunk* FirstDepth;
    render_lines_chunk* FirstNoDepth;
    
    int ChunksAllocated;
};

struct render_frame_info{
    float dt;
    
    int Width;
    int Height;
    
    int InitWidth;
    int InitHeight;
    
    u32 RendererType;
    
    bmp_info* SoftwareBuffer;
};

struct render_state{
    mem_arena* Arena;
    
    render_camera_setup CameraSetups[RENDER_MAX_CAMERA_SETUPS];
    int CameraSetupsCount;
    
    render_queue Queues[RENDER_MAX_QUEUES];
    int QueuesCount;
    
    random_generation Random;
    render_platform_api PlatformRenderAPI;
    
    Asset_Atlas* CurAtlas;
    b32 IsSoftwareRenderer;
    mem_arena StackArena;
    int EntryCount;
    
    int InitWindowWidth;
    int InitWindowHeight;
    
    b32 FrameInfoIsSet;
    render_frame_info FrameInfo;
    
    u32 ToShowBufferType;
    const char* FilterNames[RenderFilter_Count];
    
    v3 SSAONoiseTexture[SSAO_NOISE_TEXTURE_SIZE];
    v3 SSAOKernelSamples[SSAO_KERNEL_MAX_SIZE];
    int SSAOKernelSampleCount;
    float SSAOKernelRadius;
    float SSAOContribution;
    float SSAORangeCheck;
    u32 SSAOFilterType;
    
    float GaussianBlur5[25];
    float GaussianBlur3[9];
    float BoxBlur3;
    float BoxBlur5; 
    
    b32 FogEnabled;
    float FogDensity;
    float FogGradient;
    v3 FogColor;
    
    render_gui_geom GuiGeom;
    render_lines_geom LinesGeom;
};


inline void* RenderPushMem(render_state* State, mi Size, mi Align = 8){
    void* Result = PushSomeMem(&State->StackArena, Size, Align);
    
    return(Result);
}

inline void* RenderPushEntry(render_state* State, u32 sizeOfType, u32 typeEnum) {
    render_entry_header* header =
		(render_entry_header*)RenderPushMem(State, sizeof(render_entry_header) + sizeOfType, RENDER_ENTRY_MEMORY_ALIGN);
    
	State->EntryCount++;
    
	header->Type = typeEnum;
	header->DataSize = sizeOfType;
    
    void* entryData = (void*)(header + 1);
    
	return(entryData);
}

#define PUSH_RENDER_ENTRY(stack, type_enum, type) (type*)RenderPushEntry(stack, sizeof(type), type_enum)


inline render_gui_geom* GetGuiGeom(render_state* State){
    render_gui_geom* Result = &State->GuiGeom;
    
    return(Result);
}

struct render_pushed_line_primitive{
    render_line_primitive* TargetArray;
    v3* TargetColorArray;
    int StartIndex;
    int Count;
    
    int LinesAlreadyAdded;
};

inline render_lines_chunk* AllocateAndInitLinesChunk(render_state* Render,
                                                     int MaxLinesCount,
                                                     b32 HasDepth)
{
    render_lines_chunk* Chunk = PushStruct(Render->Arena, render_lines_chunk);
    
    Chunk->Lines = PushArray(Render->Arena, render_line_primitive, MaxLinesCount);
    Chunk->Colors = PushArray(Render->Arena, v3, MaxLinesCount);
    
    Chunk->Count = 0;
    Chunk->MaxCount = MaxLinesCount;
    
    Chunk->Next = 0;
    
    Render->LinesGeom.ChunksAllocated++;
    
    return(Chunk);
}

inline render_pushed_line_primitive BeginLinePrimitive(render_state* Render,
                                                       int LinesCount,
                                                       f32 LineWidth,
                                                       b32 HasDepth)
{
    render_pushed_line_primitive Result = {};
    render_lines_geom* Geom = &Render->LinesGeom;
    
    render_lines_chunk** Chunk = 0;
    if(HasDepth){
        Chunk = &Geom->FirstDepth;
    }
    else{
        Chunk = &Geom->FirstNoDepth;
    }
    
    b32 NeedAlloc = false;
    render_lines_chunk* At = 0;
    if(*Chunk){
        // NOTE(Dima): Scan through all and determine if we can place
        At = *Chunk;
        
        while(At){
            
            if(At->Count + LinesCount < At->MaxCount){
                break;
            }
            
            At = At->Next;
        }
        
        if(At == 0){
            NeedAlloc = true;
        }
    }
    else{
        NeedAlloc = true;
    }
    
    
    // NOTE(Dima): If we need to allocate new chunk - allocate
    if(NeedAlloc){
        render_lines_chunk* NewChunk = AllocateAndInitLinesChunk(Render, 50000, HasDepth);
        NewChunk->Next = (*Chunk);
        *Chunk = NewChunk;
        
        At = NewChunk;
    }
    // NOTE(Dima): Now we can place in At
    
    // NOTE(Dima): Init result
    Result.TargetArray = At->Lines;
    Result.TargetColorArray = At->Colors;
    Result.StartIndex = At->Count;
    Result.Count = LinesCount;
    Result.LinesAlreadyAdded = 0;
    
    At->Count += LinesCount;
    
    return(Result);
}

inline void AddLine(render_pushed_line_primitive* Prim, 
                    v3 Start, v3 End, v3 ColorRGB)
{
    Assert(Prim->LinesAlreadyAdded < Prim->Count);
    if(Prim->LinesAlreadyAdded < Prim->Count){
        int TargetIndex = Prim->StartIndex + Prim->LinesAlreadyAdded++;
        render_line_primitive* Target = &Prim->TargetArray[TargetIndex];
        
        Target->Start = Start;
        Target->End = End;
        
        Prim->TargetColorArray[TargetIndex] = ColorRGB;
    }
}

inline void PushLine(render_state* Render, 
                     v3 Start, 
                     v3 End, 
                     v3 ColorRGB,
                     f32 LineWidth = 1.0f,
                     b32 HasDepth = true)
{
    render_pushed_line_primitive Prim = BeginLinePrimitive(Render, 1, LineWidth, HasDepth);
    AddLine(&Prim, Start, End, ColorRGB);
}

inline void PushGuiLine(render_state* Render,
                        v2 From, v2 To, v4 Color)
{
    render_gui_geom* Geom = &Render->GuiGeom;
    
    Geom->LinePoints[Geom->LinePointsCount++] = From;
    Geom->LinePoints[Geom->LinePointsCount++] = To;
    
    Geom->LineColors[Geom->LineColorsCount++] = Color;
}

inline void PushGuiLineRect(render_state* Render,
                            rc2 Rect, v4 Color)
{
    v2 P0 = Rect.Min;
    v2 P1 = V2(Rect.Max.x, Rect.Min.y);
    v2 P2 = Rect.Max;
    v2 P3 = V2(Rect.Min.x, Rect.Max.y);
    
    PushGuiLine(Render, P0, P1, Color);
    PushGuiLine(Render, P1, P2, Color);
    PushGuiLine(Render, P2, P3, Color);
    PushGuiLine(Render, P3, P0, Color);
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
    
    v2 Min = Rect.Min;
    v2 Max = Rect.Max;
    
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
    v2 Min = rc.Min;
    v2 Max = rc.Max;
    
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
    v2 p = rect.Min;
    v2 dim = GetRectDim(rect);
    
    PushGuiGeom_RectOutline(Geom, p, dim, pixelWidth, Color);
}

inline void PushGuiGeom_InnerOutline(render_gui_geom* Geom, rc2 rect, int pixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
    v2 p = rect.Min + V2(pixelWidth, pixelWidth);
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushGuiGeom_RectOutline(Geom, p, dim, pixelWidth, Color);
}


inline void PushClearColor(render_state* State, v3 color){
    render_entry_clear_color* entry = PUSH_RENDER_ENTRY(State, RenderEntry_ClearColor, render_entry_clear_color);;
    
    entry->clearColor01 = color;
}

inline void PushBitmap(render_state* State, render_primitive_bitmap* bitmap, v2 p, float height, v4 multColor){
    render_entry_bitmap* entry = PUSH_RENDER_ENTRY(State, RenderEntry_Bitmap, render_entry_bitmap);
    
    entry->Bitmap = bitmap;
    entry->P = p;
    entry->PixelHeight = height;
    entry->ModulationColor01 = multColor;
}

inline void PushGradient(render_state* State, 
                         rc2 rc, 
                         v3 color1, 
                         v3 color2, 
                         u32 gradType)
{
    if(State->IsSoftwareRenderer){
        render_entry_gradient* entry = PUSH_RENDER_ENTRY(State, RenderEntry_Gradient, render_entry_gradient);
        
        entry->rc = rc;
        entry->color1 = color1;
        entry->color2 = color2;
        entry->gradType = gradType;
    }
    else{
        PushGuiGeom_Gradient(GetGuiGeom(State), rc, color1, color2, gradType);
    }
}

inline void PushGradient(render_state* State, 
                         rc2 rc, 
                         v4 color1, 
                         v4 color2, 
                         u32 gradType)
{
    PushGradient(State, rc, color1.rgb, color2.rgb, gradType);
}

inline void PushRect(
                     render_state* State, 
                     rc2 rect, 
                     v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(State->IsSoftwareRenderer){
        render_entry_rect* entry = PUSH_RENDER_ENTRY(State, RenderEntry_Rect, render_entry_rect);
        
        rect = RectNormalizeSubpixel(rect);
        
        entry->p = rect.Min;
        entry->dim = GetRectDim(rect);
        entry->modulationColor01 = multColor;
    }
    else{
        PushGuiGeom_Rect(&State->GuiGeom, rect, multColor);
    }
}

inline void PushRect(render_state* State, 
                     v2 p, v2 dim,
                     v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 rect = RcMinDim(p, dim);
    
    PushRect(State, rect, multColor);
}

inline void PushRectOutline(render_state* State, v2 p, v2 dim, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    if(State->IsSoftwareRenderer){
        v2 widthQuad = V2(pixelWidth, pixelWidth);
        PushRect(State, V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), multColor);
        PushRect(State, V2(p.x - pixelWidth, p.y), V2(pixelWidth, dim.y + pixelWidth), multColor);
        PushRect(State, V2(p.x, p.y + dim.y), V2(dim.x + pixelWidth, pixelWidth), multColor);
        PushRect(State, V2(p.x + dim.x, p.y), V2(pixelWidth, dim.y), multColor);
    }
    else{
        PushGuiGeom_RectOutline(&State->GuiGeom, p, dim, pixelWidth, multColor);
    }
}

inline void PushRectOutline(render_state* State, rc2 rect, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.Min;
    v2 dim = GetRectDim(rect);
    
    PushRectOutline(State, p, dim, pixelWidth, multColor);
}

inline void PushRectInnerOutline(render_state* State, rc2 rect, int pixelWidth, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.Min + V2(pixelWidth, pixelWidth);;
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushRectOutline(State, p, dim, pixelWidth, color);
}

inline void PushGlyph(render_state* State, 
                      v2 P, v2 Dim, 
                      render_primitive_bitmap* Bitmap, 
                      v2 MinUV, v2 MaxUV,
                      v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(State->IsSoftwareRenderer){
        render_entry_bitmap* entry = PUSH_RENDER_ENTRY(State, RenderEntry_Bitmap, render_entry_bitmap);
        
        entry->P = P;
        entry->PixelHeight = Dim.y;
        entry->ModulationColor01 = ModColor;
        entry->Bitmap = Bitmap;
    }
    else{
        PushGuiGeom_InAtlasBmp(&State->GuiGeom, P, Dim, MinUV, MaxUV, ModColor);
    }
}

inline void PushMesh(render_state* State,
                     render_primitive_mesh* Mesh,
                     m44 Transform,
                     render_primitive_material* Material = 0,
                     v3 Color = V3(1.0f, 0.0f, 1.0f),
                     m44* BoneTransforms = 0,
                     int BoneCount = 0)
{
    
    render_entry_mesh* entry = PUSH_RENDER_ENTRY(State, RenderEntry_Mesh, render_entry_mesh);
    
    entry->Mesh = Mesh;
    entry->BoneCount = BoneCount;
    entry->BoneTransforms = BoneTransforms;
    entry->ModColor = Color;
    entry->Material = Material;
    entry->Transform = Transform;
}

inline void PushMesh(render_state* State,
                     render_primitive_mesh* Mesh,
                     v3 P, quat R, v3 S,
                     v3 Color = V3(1.0f, 0.0f, 1.0f))
{
    m44 Matrix = ScalingMatrix(S) * RotationMatrix(R) * TranslationMatrix(P);
    PushMesh(State, Mesh, Matrix);
}

inline b32 CameraSetupIsValid(render_state* Render, int SetupIndex){
    b32 Result = (SetupIndex < Render->CameraSetupsCount) && (SetupIndex >= 0);
    
    return(Result);
}

inline b32 RenderQueueIsValid(render_state* Render, int QueueIndex){
    b32 Result = (QueueIndex < Render->QueuesCount) && (QueueIndex >= 0) && (QueueIndex < RENDER_MAX_QUEUES);
    
    return(Result);
}

inline b32 RenderQueueIsEmpty(render_state* Render, int QueueIndex){
    b32 Result = false;
    
    if(RenderQueueIsValid(Render, QueueIndex)){
        render_queue* Q = &Render->Queues[QueueIndex];
        
        Result = (Q->FirstEntry >= Q->OnePastLastEntry) || (Q->FirstEntry == 0) || (Q->OnePastLastEntry == 0);
    }
    
    return(Result);
}

// NOTE(Dima): Returns true if we need to render
inline b32 ProcessQueueEntry(render_state* Render, 
                             int BeginnedQueueIndex,
                             u32 PassType,
                             void* AtBeforeInc)
{
    b32 Result = false;
    
    b32 QueueValid = RenderQueueIsValid(Render, BeginnedQueueIndex);
    
    if(QueueValid && (PassType == RenderPass_Main)){
        render_queue* Queue = &Render->Queues[BeginnedQueueIndex];
        
        if(Queue->FirstEntry == 0){
            Queue->FirstEntry = AtBeforeInc;
        }
    }
    else{
        Result = true;
    }
    
    return(Result);
}

inline void PushBeginQueue(render_state* Render,
                           int QueueIndex)
{
    b32 IsValid = RenderQueueIsValid(Render, QueueIndex);
    Assert(IsValid);
    
    if(IsValid){
        render_entry_renderqueue* Entry = PUSH_RENDER_ENTRY(Render, 
                                                            RenderEntry_BeginQueue, 
                                                            render_entry_renderqueue);
        
        Entry->QueueIndex = QueueIndex;
    }
}

inline void PushEndQueue(render_state* Render,
                         int QueueIndex)
{
    b32 IsValid = RenderQueueIsValid(Render, QueueIndex);
    Assert(IsValid);
    
    if(IsValid){
        render_entry_renderqueue* Entry = PUSH_RENDER_ENTRY(Render, 
                                                            RenderEntry_EndQueue, 
                                                            render_entry_renderqueue);
        
        Entry->QueueIndex = QueueIndex;
    }
}

inline void PushRenderPass(render_state* Render,
                           int CameraSetupIndex,
                           int QueueIndex)
{
    b32 SetupValid = CameraSetupIsValid(Render, CameraSetupIndex);
    b32 QueueIsValid = QueueIndex < Render->QueuesCount && RenderQueueIsValid(Render, QueueIndex);
    
    Assert(QueueIsValid && SetupValid);
    
    if(QueueIsValid && SetupValid){
        render_entry_renderpass* Entry = PUSH_RENDER_ENTRY(Render,
                                                           RenderEntry_RenderPass,
                                                           render_entry_renderpass);
        
        Entry->CameraSetupIndex = CameraSetupIndex;
        Entry->QueueIndex = QueueIndex;
    }
}

inline void PushGuiChunk(render_state* State, int ChunkIndex){
    render_entry_gui_chunk* Entry = PUSH_RENDER_ENTRY(State, RenderEntry_GuiGeometryChunk, render_entry_gui_chunk);
    
    Entry->ChunkIndex = ChunkIndex;
}

inline void BeginSetupChunk(render_state* State, render_gui_geom* Geom){
    
    render_gui_chunk* CurChunk = &Geom->Chunks[Geom->CurChunkIndex];
    
    CurChunk->IndicesCount = Geom->IndicesCount - CurChunk->StartIndicesCount;
    CurChunk->LinePointsCount = Geom->LinePointsCount - CurChunk->LinePointsBase;
    
    if((CurChunk->IndicesCount > 0) ||
       (CurChunk->LinePointsCount > 0))
    {
        PushGuiChunk(State, Geom->CurChunkIndex);
    }
    
    // NOTE(Dima): Advancing current chunk index
    ASSERT(Geom->CurChunkIndex + 1 < MAX_GUI_CHUNKS);
    Geom->CurChunkIndex = Geom->CurChunkIndex + 1;
}

inline void SetupNewChunk(render_gui_geom* Geom, 
                          rc2 ClipRect, 
                          render_gui_chunk* Chunk)
{
    Chunk->IndicesCount = 0;
    Chunk->LinePointsCount = 0;
    Chunk->BaseVertex = Geom->VerticesCount;
    Chunk->LinePointsBase = Geom->LinePointsCount;
    Chunk->ClipRect = ClipRect;
    Chunk->StartIndicesCount = Geom->IndicesCount;
}

inline void BeginGuiChunk(render_state* State, rc2 ClipRect){
    render_gui_geom* Geom = &State->GuiGeom;
    
    BeginSetupChunk(State, Geom);
    
    // NOTE(Dima): Setting new chunk
    SetupNewChunk(Geom, ClipRect, &Geom->Chunks[Geom->CurChunkIndex]);
    
    // NOTE(Dima): Adding new clip rect to stack
    ++Geom->ClipRectStackIndex;
    ASSERT(Geom->ClipRectStackIndex < MAX_CLIP_RECT_STACK_DEPTH);
    Geom->ClipRectStack[Geom->ClipRectStackIndex] = ClipRect;
}

inline void EndGuiChunk(render_state* State){
    render_gui_geom* Geom = &State->GuiGeom;
    
    BeginSetupChunk(State, Geom);
    
    // NOTE(Dima): Popping last value from ClipRect's stack
    ASSERT(Geom->ClipRectStackIndex > 0);
    --Geom->ClipRectStackIndex;
    
    // NOTE(Dima): Setting new chunk
    SetupNewChunk(Geom, 
                  Geom->ClipRectStack[Geom->ClipRectStackIndex],
                  &Geom->Chunks[Geom->CurChunkIndex]);
}

// NOTE(Dima): RENDERER API
inline void RenderSetFrameInfo(render_state* Render, render_frame_info FrameInfo){
    Render->FrameInfo = FrameInfo;
    Render->FrameInfoIsSet = 1;
    
    Render->PlatformRenderAPI.RendererType = FrameInfo.RendererType;
}

#endif
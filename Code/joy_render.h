#ifndef JOY_RENDER_H
#define JOY_RENDER_H

#include "joy_render_stack.h"
#include "joy_memory.h"

struct Render_Gui_Geom_Vertex{
    float Data[8];
};

enum Render_Gui_Geom_Type{
    RenderGuiGeom_Rect,
    RenderGuiGeom_Bmp,
};

struct Render_Gui_Geom{
    Render_Gui_Geom_Vertex* Vertices;
    u32* Indices;
    u8* TriangleGeomTypes;
    int TriangleGeomTypesCount;
    int VerticesCount;
    int IndicesCount;
    
    int MaxVerticesCount;
    int MaxIndicesCount;
    int MaxTriangleGeomTypesCount;
};

struct Render_Frame_Info{
    float dt;
    int Width;
    int Height;
    
    u32 RendererType;
    
    Bmp_Info* SoftwareBuffer;
};

enum Renderer_Type{
    Renderer_OpenGL,
    Renderer_Software,
};

struct Render_State{
    Memory_Region* MemRegion;
    
    Render_Stack Stacks[16];
    int StacksCount;
    
    b32 FrameInfoIsSet;
    Render_Frame_Info FrameInfo;
    
    u32 RendererType;
    
    Render_Gui_Geom GuiGeom;
};

inline Render_Gui_Geom* GetGuiGeom(Render_Stack* RenderStack){
    Render_Gui_Geom* Result = &RenderStack->Render->GuiGeom;
    
    return(Result);
}

inline void PushGuiGeom_Internal(Render_Gui_Geom* geom, 
                                 Render_Gui_Geom_Vertex arr[4], 
                                 u8 GeomType)
{
    
    u32 VAt = geom->VerticesCount;
    geom->Vertices[geom->VerticesCount++] = arr[0];
    geom->Vertices[geom->VerticesCount++] = arr[1];
    geom->Vertices[geom->VerticesCount++] = arr[2];
    geom->Vertices[geom->VerticesCount++] = arr[3];
    
    u32 At = geom->IndicesCount;
    geom->Indices[At + 0] = VAt + 0;
    geom->Indices[At + 1] = VAt + 1;
    geom->Indices[At + 2] = VAt + 2;
    geom->Indices[At + 3] = VAt + 0;
    geom->Indices[At + 4] = VAt + 2;
    geom->Indices[At + 5] = VAt + 3;
    geom->IndicesCount += 6;
    
    u32 GeomTypeAt = geom->TriangleGeomTypesCount;
    geom->TriangleGeomTypes[GeomTypeAt + 0] = GeomType;
    geom->TriangleGeomTypes[GeomTypeAt + 1] = GeomType;
    geom->TriangleGeomTypesCount += 2;
}

inline void PushGuiGeom_InAtlasBmp(Render_Gui_Geom* geom, 
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
    
    Render_Gui_Geom_Vertex RectArr[] = {
        {Min.x, Max.y, MinUV.x, MaxUV.y, r, g, b, a },
        {Max.x, Max.y, MaxUV.x, MaxUV.y, r, g, b, a },
        {Max.x, Min.y, MaxUV.x, MinUV.y, r, g, b, a },
        {Min.x, Min.y, MinUV.x, MinUV.y, r, g, b, a },
    };
    
    PushGuiGeom_Internal(geom, RectArr, RenderGuiGeom_Bmp);
}

inline void PushGuiGeom_InAtlasBmp(Render_Gui_Geom* geom, 
                                   v2 P, v2 Dim, 
                                   In_Atlas_Bitmap bmp, 
                                   v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    PushGuiGeom_InAtlasBmp(geom, P, Dim, bmp.MinUV, bmp.MaxUV, Color);
}

inline void PushGuiGeom_Rect(Render_Gui_Geom* geom, 
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
    
    Render_Gui_Geom_Vertex RectArr[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r, g, b, a},
        {Max.x, Max.y, 1.0f, 1.0f, r, g, b, a},
        {Max.x, Min.y, 1.0f, 0.0f, r, g, b, a},
        {Min.x, Min.y, 0.0f, 0.0f, r, g, b, a},
    };
    
    PushGuiGeom_Internal(geom, RectArr, RenderGuiGeom_Rect);
}

inline void PushGuiGeom_Rect(Render_Gui_Geom* geom, 
                             v2 p, v2 dim,
                             v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 Rect = RcMinDim(p, dim);
    
    PushGuiGeom_Rect(geom, Rect, multColor);
}

inline void PushGuiGeom_Gradient(Render_Gui_Geom* geom, 
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
    
    Render_Gui_Geom_Vertex RectArr1[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r1, g1, b1, 1.0f},
        {Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Min.y, 1.0f, 0.0f, r2, g2, b2, 1.0f},
        {Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f},
    };
    
    Render_Gui_Geom_Vertex RectArr2[] = {
        {Min.x, Max.y, 0.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f},
        {Max.x, Min.y, 1.0f, 0.0f, r1, g1, b1, 1.0f},
        {Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f},
    };
    
    Render_Gui_Geom_Vertex *ArrToUse = 0;
    if(gradType == RenderEntryGradient_Horizontal){
        ArrToUse = RectArr1;
    }
    else if(gradType == RenderEntryGradient_Vertical){
        ArrToUse = RectArr2;
    }
    else{
        INVALID_CODE_PATH;
    }
    
    PushGuiGeom_Internal(geom, ArrToUse, RenderGuiGeom_Rect);
}

inline void PushGuiGeom_Gradient(Render_Gui_Geom* geom, 
                                 rc2 rc, 
                                 v4 color1, 
                                 v4 color2, 
                                 u32 gradType)
{
    PushGuiGeom_Gradient(geom, rc,
                         color1.rgb, color2.rgb,
                         gradType);
}


inline void PushGuiGeom_RectOutline(Render_Gui_Geom* geom, 
                                    v2 p, v2 dim, 
                                    int pixelWidth, v4 multColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) 
{
    PushGuiGeom_Rect(geom, 
                     V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(geom, 
                     V2(p.x - pixelWidth, p.y), 
                     V2(pixelWidth, dim.y + pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(geom, 
                     V2(p.x, p.y + dim.y), 
                     V2(dim.x + pixelWidth, pixelWidth), 
                     multColor);
    PushGuiGeom_Rect(geom, 
                     V2(p.x + dim.x, p.y), 
                     V2(pixelWidth, dim.y), 
                     multColor);
}


inline void PushGuiGeom_RectOutline(Render_Gui_Geom* geom, rc2 rect, 
                                    int pixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) 
{
    v2 p = rect.min;
    v2 dim = GetRectDim(rect);
    
    PushGuiGeom_RectOutline(geom, p, dim, pixelWidth, Color);
}

inline void PushGuiGeom_InnerOutline(Render_Gui_Geom* geom, rc2 rect, int pixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
    v2 p = rect.min + V2(pixelWidth, pixelWidth);
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushGuiGeom_RectOutline(geom, p, dim, pixelWidth, Color);
}


inline void PushClearColor(Render_Stack* stack, v3 color){
    RenderEntryClearColor* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_ClearColor, RenderEntryClearColor);;
    
    entry->clearColor01 = color;
}

inline void PushBitmap(Render_Stack* stack, Bmp_Info* bitmap, v2 p, float height, v4 multColor){
    RenderEntryBitmap* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Bitmap, RenderEntryBitmap);
    
    entry->Bitmap = bitmap;
    entry->P = p;
    entry->PixelHeight = height;
    entry->ModulationColor01 = multColor;
}

inline void PushGradient(Render_Stack* stack, 
                         rc2 rc, 
                         v3 color1, 
                         v3 color2, 
                         u32 gradType)
{
    if(stack->IsSoftwareRenderer){
        RenderEntryGradient* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Gradient, RenderEntryGradient);
        
        entry->rc = rc;
        entry->color1 = color1;
        entry->color2 = color2;
        entry->gradType = gradType;
    }
    else{
        PushGuiGeom_Gradient(GetGuiGeom(stack), rc, color1, color2, gradType);
    }
}

inline void PushGradient(Render_Stack* stack, 
                         rc2 rc, 
                         v4 color1, 
                         v4 color2, 
                         u32 gradType)
{
    PushGradient(stack, rc, color1.rgb, color2.rgb, gradType);
}

inline void PushRect(
Render_Stack* stack, 
rc2 rect, 
v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(stack->IsSoftwareRenderer){
        RenderEntryRect* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Rect, RenderEntryRect);
        
        rect = RectNormalizeSubpixel(rect);
        
        entry->p = rect.min;
        entry->dim = GetRectDim(rect);
        entry->modulationColor01 = multColor;
    }
    else{
        PushGuiGeom_Rect(GetGuiGeom(stack), rect, multColor);
    }
}

inline void PushRect(Render_Stack* stack, 
                     v2 p, v2 dim,
                     v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 rect = RcMinDim(p, dim);
    
    PushRect(stack, rect, multColor);
}

inline void PushRectOutline(Render_Stack* stack, v2 p, v2 dim, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    if(stack->IsSoftwareRenderer){
        v2 widthQuad = V2(pixelWidth, pixelWidth);
        PushRect(stack, V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), multColor);
        PushRect(stack, V2(p.x - pixelWidth, p.y), V2(pixelWidth, dim.y + pixelWidth), multColor);
        PushRect(stack, V2(p.x, p.y + dim.y), V2(dim.x + pixelWidth, pixelWidth), multColor);
        PushRect(stack, V2(p.x + dim.x, p.y), V2(pixelWidth, dim.y), multColor);
    }
    else{
        PushGuiGeom_RectOutline(GetGuiGeom(stack), p, dim, pixelWidth, multColor);
    }
}

inline void PushRectOutline(Render_Stack* stack, rc2 rect, int pixelWidth, v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.min;
    v2 dim = GetRectDim(rect);
    
    PushRectOutline(stack, p, dim, pixelWidth, multColor);
}

inline void PushRectInnerOutline(Render_Stack* stack, rc2 rect, int pixelWidth, v4 color = V4(1.0f, 1.0f, 1.0f, 1.0f)) {
    v2 p = rect.min + V2(pixelWidth, pixelWidth);;
    v2 dim = GetRectDim(rect) - 2.0f * V2(pixelWidth, pixelWidth);
    
    PushRectOutline(stack, p, dim, pixelWidth, color);
}

inline void PushGlyph(Render_Stack* stack, 
                      v2 P, v2 Dim, 
                      Bmp_Info* Bitmap, 
                      v2 MinUV, v2 MaxUV,
                      v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    if(stack->IsSoftwareRenderer){
        RenderEntryBitmap* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Bitmap, RenderEntryBitmap);
        
        entry->P = P;
        entry->PixelHeight = Dim.y;
        entry->ModulationColor01 = ModColor;
        entry->Bitmap = Bitmap;
    }
    else{
        PushGuiGeom_InAtlasBmp(GetGuiGeom(stack), P, Dim, MinUV, MaxUV, ModColor);
    }
}


// NOTE(Dima): RENDERER API
inline void RenderSetFrameInfo(Render_State* render, Render_Frame_Info frameInfo){
    render->FrameInfo = frameInfo;
    render->FrameInfoIsSet = 1;
    
    render->RendererType = frameInfo.RendererType;
}

void RenderInit(Render_State* render);
Render_Stack* RenderAddStack(Render_State* render, char* Name, mi Size = Megabytes(1));
void RenderBeginFrame(Render_State* render);
void RenderEndFrame(Render_State* render);

#endif
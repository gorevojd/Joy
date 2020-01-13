#ifndef JOY_RENDER_STACK_H
#define JOY_RENDER_STACK_H

#include "joy_types.h"
#include "joy_memory.h"
#include "joy_math.h"
#include "joy_asset_types.h"

enum RenderEntryType{
    RenderEntry_ClearColor,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
    RenderEntry_Glyph,
    RenderEntry_Mesh,
    RenderEntry_Gradient,
};

struct Render_Entry_Header{
    u32 type;
    u32 dataSize;
};

#define RENDER_GET_ENTRY(type) type* entry = (type*)at
struct Render_Stack{
    void* mem;
    mi memUsed;
    mi memSize;
    
    float Width;
    float Height;
    
    int entryCount;
};

inline Render_Stack InitRenderStack(void* mem4stack, mi size){
    Render_Stack result = {};
    
    result.mem = mem4stack;
    result.memUsed = 0;
    result.memSize = size;
    result.entryCount = 0;
    
    return(result);
}

inline void RenderStackBeginFrame(Render_Stack* stack){
    stack->memUsed = 0;
    stack->entryCount = 0;
}

#define RENDER_ENTRY_MEMORY_ALIGN 4
#pragma pack(push, RENDER_ENTRY_MEMORY_ALIGN)
struct RenderEntryClearColor{
    v3 clearColor01;
};

struct RenderEntryBitmap{
    Bmp_Info* bitmap;
    v2 topLeftP;
    float pixelHeight;
    v4 modulationColor01;
};

enum RenderEntryGradientType{
    RenderEntryGradient_Horizontal,
    RenderEntryGradient_Vertical,
};

struct RenderEntryGradient{
    rc2 rc;
    v3 color1;
    v3 color2;
    u32 gradType;
};

struct RenderEntryRect{
    v4 modulationColor01;
    v2 p;
    v2 dim;
};

struct RenderEntryGlyph{
    u32 GlyphIndex;
    
    v2 P;
    v2 Dim;
    
    v4 ModColor;
};

struct RenderEntryMesh{
    
};
#pragma pack(pop)

inline void* RenderPushMem(Render_Stack* stack, mi size, mi align = 8){
    mi beforeAlign = (mi)stack->mem + stack->memUsed;
    mi alignedPos = (beforeAlign + align - 1) & (~(align - 1));
    mi advancedByAlign = alignedPos - beforeAlign;
    
    mi toAllocateSize = advancedByAlign + size;
    mi newUsedCount = stack->memUsed + toAllocateSize;
    
    void* result = 0;
    
    Assert(newUsedCount <= stack->memSize);
    
    result = (void*)alignedPos;
    stack->memUsed= newUsedCount;
    
    return(result);
    
}


inline void* RenderPushEntryToStack(Render_Stack* stack, u32 sizeOfType, u32 typeEnum) {
	Render_Entry_Header* header =
		(Render_Entry_Header*)RenderPushMem(stack, sizeof(Render_Entry_Header) + sizeOfType, RENDER_ENTRY_MEMORY_ALIGN);
    
	stack->entryCount++;
    
	header->type = typeEnum;
	header->dataSize = sizeOfType;
    
    void* entryData = (void*)(header + 1);
    
	return(entryData);
}

#define PUSH_RENDER_ENTRY(stack, type_enum, type) (type*)RenderPushEntryToStack(stack, sizeof(type), type_enum)

inline void PushClearColor(Render_Stack* stack, v3 color){
    RenderEntryClearColor* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_ClearColor, RenderEntryClearColor);;
    
    entry->clearColor01 = color;
}

inline void PushBitmap(Render_Stack* stack, Bmp_Info* bitmap, v2 p, float height, v4 multColor){
    RenderEntryBitmap* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Bitmap, RenderEntryBitmap);
    
    entry->bitmap = bitmap;
    entry->topLeftP = p;
    entry->pixelHeight = height;
    entry->modulationColor01 = multColor;
}

inline void PushGradient(Render_Stack* stack, 
                         rc2 rc, 
                         v3 color1, 
                         v3 color2, 
                         u32 gradType)
{
    RenderEntryGradient* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Gradient, RenderEntryGradient);
    
    entry->rc = rc;
    entry->color1 = color1;
    entry->color2 = color2;
    entry->gradType = gradType;
}

inline void PushGradient(Render_Stack* stack, 
                         rc2 rc, 
                         v4 color1, 
                         v4 color2, 
                         u32 gradType)
{
    RenderEntryGradient* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Gradient, RenderEntryGradient);
    
    entry->rc = rc;
    entry->color1 = color1.rgb;
    entry->color2 = color2.rgb;
    entry->gradType = gradType;
}

inline void PushRect(
Render_Stack* stack, 
rc2 rect, 
v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    RenderEntryRect* entry = PUSH_RENDER_ENTRY(stack, RenderEntry_Rect, RenderEntryRect);
    
    rect = RectNormalizeSubpixel(rect);
    
	entry->p = rect.min;
	entry->dim = GetRectDim(rect);
	entry->modulationColor01 = multColor;
}

inline void PushRect(Render_Stack* stack, 
                     v2 p, v2 dim,
                     v4 multColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 rect = RcMinDim(p, dim);
    
    PushRect(stack, rect, multColor);
}

inline void PushRectOutline(Render_Stack* stack, v2 p, v2 dim, int pixelWidth, v4 multColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
	v2 widthQuad = V2(pixelWidth, pixelWidth);
	PushRect(stack, V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), multColor);
	PushRect(stack, V2(p.x - pixelWidth, p.y), V2(pixelWidth, dim.y + pixelWidth), multColor);
	PushRect(stack, V2(p.x, p.y + dim.y), V2(dim.x + pixelWidth, pixelWidth), multColor);
	PushRect(stack, V2(p.x + dim.x, p.y), V2(pixelWidth, dim.y), multColor);
}


inline void PushRectOutline(Render_Stack* stack, rc2 rect, int pixelWidth, v4 multColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
    v2 p = rect.min;
    v2 dim = GetRectDim(rect);
    
	v2 widthQuad = V2(pixelWidth, pixelWidth);
	PushRect(stack, V2(p.x - pixelWidth, p.y - pixelWidth), V2(dim.x + 2.0f * pixelWidth, pixelWidth), multColor);
	PushRect(stack, V2(p.x - pixelWidth, p.y), V2(pixelWidth, dim.y + pixelWidth), multColor);
	PushRect(stack, V2(p.x, p.y + dim.y), V2(dim.x + pixelWidth, pixelWidth), multColor);
	PushRect(stack, V2(p.x + dim.x, p.y), V2(pixelWidth, dim.y), multColor);
}

inline void PushRectInnerOutline(Render_Stack* stack, rc2 rect, int pixelWidth, v4 color = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
	v2 dim = GetRectDim(rect);
	v2 p = rect.min;
    
	PushRect(stack, V2(p.x, p.y), V2(dim.x, pixelWidth), color);
	PushRect(stack, V2(p.x, p.y + pixelWidth), V2(pixelWidth, dim.y - pixelWidth), color);
	PushRect(stack, V2(p.x + pixelWidth, p.y + dim.y - pixelWidth), V2(dim.x - pixelWidth, pixelWidth), color);
	PushRect(stack, V2(p.x + dim.x - pixelWidth, p.y + pixelWidth), V2(pixelWidth, dim.y - 2 * pixelWidth), color);
}


#endif
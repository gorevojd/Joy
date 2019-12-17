#ifndef JOY_RENDER_STACK_H
#define JOY_RENDER_STACK_H

#include "joy_types.h"
#include "joy_memory.h"
#include "joy_math.h"
#include "joy_asset_types.h"

enum render_entry_type{
    RenderEntry_ClearColor,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
    RenderEntry_Glyph,
    RenderEntry_Mesh,
    RenderEntry_Gradient,
};

struct render_entry_header{
    u32 Type;
    u32 DataSize;
};

#define RENDER_GET_ENTRY(type) type* Entry = (type*)At
struct render_stack{
    memory_region Data;
    
    int EntryCount;
};

inline render_stack InitRenderStack(memory_region* Stack, size_t StackByteSize){
    render_stack Result = {};
    
    Result.Data = SplitMemoryRegion(Stack, StackByteSize);
    Result.EntryCount = 0;
    
    return(Result);
}

inline void RenderStackBeginFrame(render_stack* Stack){
    Stack->Data.Used = 0;
    Stack->EntryCount = 0;
}

#define RENDER_ENTRY_MEMORY_ALIGN 4
#pragma pack(push, RENDER_ENTRY_MEMORY_ALIGN)
struct render_entry_clear_color{
    v3 ClearColor01;
};

struct render_entry_bitmap{
    bmp_info* Bitmap;
    v2 TopLeftP;
    float PixelHeight;
    v4 ModulationColor01;
};

enum render_entry_gradient_type{
    RenderEntryGradient_Horizontal,
    RenderEntryGradient_Vertical,
};

struct render_entry_gradient{
    rc2 Rc;
    v3 Color1;
    v3 Color2;
    u32 GradientType;
};

struct render_entry_rect{
    v4 ModulationColor01;
    v2 P;
    v2 Dim;
};

struct render_entry_glyph{
    
};

struct render_entry_mesh{
    
};
#pragma pack(pop)

inline void* RenderPushEntryToStack(render_stack* Stack, u32 SizeOfType, u32 TypeEnum) {
	render_entry_header* Header =
		(render_entry_header*)PushSomeMem(&Stack->Data, sizeof(render_entry_header) + SizeOfType, RENDER_ENTRY_MEMORY_ALIGN);
    
	Stack->EntryCount++;
    
	Header->Type = TypeEnum;
	Header->DataSize = SizeOfType;
    
    void* EntryData = (void*)(Header + 1);
    
	return(EntryData);
}

#define PUSH_RENDER_ENTRY(stack, type_enum, type) (type*)RenderPushEntryToStack(stack, sizeof(type), type_enum)

inline void PushClearColor(render_stack* Stack, v3 Color){
    render_entry_clear_color* Entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_ClearColor, render_entry_clear_color);;
    
    Entry->ClearColor01 = Color;
}

inline void PushBitmap(render_stack* Stack, bmp_info* Bitmap, v2 P, float Height, v4 MultColor){
    render_entry_bitmap* Entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Bitmap, render_entry_bitmap);
    
    Entry->Bitmap = Bitmap;
    Entry->TopLeftP = P;
    Entry->PixelHeight = Height;
    Entry->ModulationColor01 = MultColor;
}

inline void PushGradient(render_stack* Stack, 
                         rc2 Rc, 
                         v3 Color1, 
                         v3 Color2, 
                         u32 GradientType)
{
    render_entry_gradient* Entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Gradient, render_entry_gradient);
    
    Entry->Rc = Rc;
    Entry->Color1 = Color1;
    Entry->Color2 = Color2;
    Entry->GradientType = GradientType;
}

inline void PushRect(
render_stack* Stack, 
rc2 Rect, 
v4 MultColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    render_entry_rect* Entry = PUSH_RENDER_ENTRY(Stack, RenderEntry_Rect, render_entry_rect);
    
    Rect = RectNormalizeSubpixel(Rect);
    
	Entry->P = Rect.Min;
	Entry->Dim = GetRectDim(Rect);
	Entry->ModulationColor01 = MultColor;
}

inline void PushRect(render_stack* Stack, 
                     v2 P, v2 Dim,
                     v4 MultColor = V4(1.0f, 1.0f, 1.0f, 1.0f)) 
{
    rc2 Rect = RcMinDim(P, Dim);
    
    PushRect(Stack, Rect, MultColor);
}

inline void PushRectOutline(render_stack* Stack, v2 P, v2 Dim, int PixelWidth, v4 MultColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
	v2 WidthQuad = V2(PixelWidth, PixelWidth);
	PushRect(Stack, V2(P.x - PixelWidth, P.y - PixelWidth), V2(Dim.x + 2.0f * PixelWidth, PixelWidth), MultColor);
	PushRect(Stack, V2(P.x - PixelWidth, P.y), V2(PixelWidth, Dim.y + PixelWidth), MultColor);
	PushRect(Stack, V2(P.x, P.y + Dim.y), V2(Dim.x + PixelWidth, PixelWidth), MultColor);
	PushRect(Stack, V2(P.x + Dim.x, P.y), V2(PixelWidth, Dim.y), MultColor);
}


inline void PushRectOutline(render_stack* Stack, rc2 Rect, int PixelWidth, v4 MultColor = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
    v2 P = Rect.Min;
    v2 Dim = GetRectDim(Rect);
    
	v2 WidthQuad = V2(PixelWidth, PixelWidth);
	PushRect(Stack, V2(P.x - PixelWidth, P.y - PixelWidth), V2(Dim.x + 2.0f * PixelWidth, PixelWidth), MultColor);
	PushRect(Stack, V2(P.x - PixelWidth, P.y), V2(PixelWidth, Dim.y + PixelWidth), MultColor);
	PushRect(Stack, V2(P.x, P.y + Dim.y), V2(Dim.x + PixelWidth, PixelWidth), MultColor);
	PushRect(Stack, V2(P.x + Dim.x, P.y), V2(PixelWidth, Dim.y), MultColor);
}

inline void PushRectInnerOutline(render_stack* Stack, rc2 Rect, int PixelWidth, v4 Color = V4(0.0f, 0.0f, 0.0f, 1.0f)) {
	v2 Dim = GetRectDim(Rect);
	v2 P = Rect.Min;
    
	PushRect(Stack, V2(P.x, P.y), V2(Dim.x, PixelWidth), Color);
	PushRect(Stack, V2(P.x, P.y + PixelWidth), V2(PixelWidth, Dim.y - PixelWidth), Color);
	PushRect(Stack, V2(P.x + PixelWidth, P.y + Dim.y - PixelWidth), V2(Dim.x - PixelWidth, PixelWidth), Color);
	PushRect(Stack, V2(P.x + Dim.x - PixelWidth, P.y + PixelWidth), V2(PixelWidth, Dim.y - 2 * PixelWidth), Color);
}


#endif
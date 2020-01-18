#ifndef JOY_RENDER_STACK_H
#define JOY_RENDER_STACK_H

#include "joy_types.h"
#include "joy_memory.h"
#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_strings.h"

enum RenderEntryType{
    RenderEntry_ClearColor,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
    RenderEntry_Mesh,
    RenderEntry_Glyph,
    RenderEntry_Gradient,
    RenderEntry_GuiGeom,
};

struct Render_Entry_Header{
    u32 type;
    u32 dataSize;
};

#define RENDER_GET_ENTRY(type) type* entry = (type*)at
struct Render_Stack{
    char Name[32];
    
    b32 IsSoftwareRenderer;
    struct Render_State* Render;
    Asset_Atlas* CurAtlas;
    
    Memory_Block MemBlock;
    int EntryCount;
};

#define RENDER_ENTRY_MEMORY_ALIGN 4
#pragma pack(push, RENDER_ENTRY_MEMORY_ALIGN)
struct RenderEntryClearColor{
    v3 clearColor01;
};

struct RenderEntryBitmap{
    Bmp_Info* Bitmap;
    v2 P;
    float PixelHeight;
    v4 ModulationColor01;
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
    v2 P;
    v2 Dim;
    
    v2 MinUV;
    v2 MaxUV;
    
    Bmp_Info* Bitmap;
    
    v4 ModColor;
};

struct RenderEntryTextureBind{
    Bmp_Info* Bitmap;
    int Unit;
};

struct RenderEntryInAtlasBmp{
    v2 P;
    v2 Dim;
    
    v2 MinUV;
    v2 MaxUV;
    
    v4 ModulationColor;
};

struct RenderEntryMesh{
    
};
#pragma pack(pop)

inline void* RenderPushMem(Render_Stack* stack, mi size, mi align = 8){
    mi beforeAlign = (mi)stack->MemBlock.Base + stack->MemBlock.Used;
    mi alignedPos = (beforeAlign + align - 1) & (~(align - 1));
    mi advancedByAlign = alignedPos - beforeAlign;
    
    mi toAllocateSize = advancedByAlign + size;
    mi newUsedCount = stack->MemBlock.Used + toAllocateSize;
    
    void* result = 0;
    
    Assert(newUsedCount <= stack->MemBlock.Total);
    
    result = (void*)alignedPos;
    stack->MemBlock.Used= newUsedCount;
    
    return(result);
    
}

inline void* RenderPushEntryToStack(Render_Stack* stack, u32 sizeOfType, u32 typeEnum) {
	Render_Entry_Header* header =
		(Render_Entry_Header*)RenderPushMem(stack, sizeof(Render_Entry_Header) + sizeOfType, RENDER_ENTRY_MEMORY_ALIGN);
    
	stack->EntryCount++;
    
	header->type = typeEnum;
	header->dataSize = sizeOfType;
    
    void* entryData = (void*)(header + 1);
    
	return(entryData);
}

#define PUSH_RENDER_ENTRY(stack, type_enum, type) (type*)RenderPushEntryToStack(stack, sizeof(type), type_enum)


#endif
#ifndef JOY_RENDER_STACK_H
#define JOY_RENDER_STACK_H

#include "joy_types.h"
#include "joy_memory.h"
#include "joy_math.h"
#include "joy_asset_types.h"
#include "joy_strings.h"

enum render_entry_type{
    RenderEntry_ClearColor,
    RenderEntry_Bitmap,
    RenderEntry_Rect,
    RenderEntry_Mesh,
    RenderEntry_Glyph,
    RenderEntry_Gradient,
    RenderEntry_GuiGeom,
    RenderEntry_GuiChunk,
};

struct render_entry_header{
    u32 type;
    u32 dataSize;
};

#define RENDER_GET_ENTRY(type) type* entry = (type*)at
struct render_stack{
    char Name[32];
    
    b32 IsSoftwareRenderer;
    struct render_state* Render;
    Asset_Atlas* CurAtlas;
    
    mem_block MemBlock;
    int EntryCount;
};

#define RENDER_ENTRY_MEMORY_ALIGN 4
#pragma pack(push, RENDER_ENTRY_MEMORY_ALIGN)
struct render_entry_clear_color{
    v3 clearColor01;
};

struct render_entry_bitmap{
    bmp_info* Bitmap;
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
    mesh_info* Mesh;
    
    m44 Transform;
};

struct render_entry_gui_chunk{
    int ChunkIndex;
};

#pragma pack(pop)

inline void* RenderPushMem(render_stack* stack, mi size, mi align = 8){
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

inline void* RenderPushEntryToStack(render_stack* stack, u32 sizeOfType, u32 typeEnum) {
	render_entry_header* header =
		(render_entry_header*)RenderPushMem(stack, sizeof(render_entry_header) + sizeOfType, RENDER_ENTRY_MEMORY_ALIGN);
    
	stack->EntryCount++;
    
	header->type = typeEnum;
	header->dataSize = sizeOfType;
    
    void* entryData = (void*)(header + 1);
    
	return(entryData);
}

#define PUSH_RENDER_ENTRY(stack, type_enum, type) (type*)RenderPushEntryToStack(stack, sizeof(type), type_enum)


#endif
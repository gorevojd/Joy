#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include "joy_platform.h"
#include "joy_types.h"
#include <stdint.h>
#include <string.h>

// TODO(Dima): Add minimum block size param to allocation function

struct mem_region{
    mem_block_entry* Block;
    
    u32 GrowSize;
};

struct mem_layer_entry{
    mem_layer_entry* Next;
    mem_layer_entry* Prev;
    
    void* Data;
    
    u32 LayerSize;
    int LayerIndex;
};

struct layered_mem{
    mem_region* Region;
    
    mem_layer_entry* LayersSentinels;
    mem_layer_entry* FreeSentinels;
    int LayersCount;
};

inline void RegionSetGrowSize(mem_region* Region, u32 GrowSize){
    Region->GrowSize = GrowSize;
}

enum memory_entry_state{
    MemoryEntry_Released,
    MemoryEntry_Used,
};

struct mem_entry{
    mem_entry* NextAlloc;
    mem_entry* PrevAlloc;
    
    mem_entry* Next;
    mem_entry* Prev;
    
    mem_entry* NextReleased;
    mem_entry* PrevReleased;
    
    // NOTE(Dima): This is internal stuff used to encounter to align
    void* _InternalData;
    u32 _InternalDataSize;
    
    // NOTE(Dima): This is actual data that user can use!
    // NOTE(Dima): It's already initialized and aligned!
    void* ActualData;
    u32 ActualDataSize;
    
    u32 State;
};

struct mem_box{
    mem_region* Region;
    mem_entry Use;
    mem_entry Free;
    
    mem_entry* First;
    mem_entry* FirstReleased;
    
    mem_box* NextBox;
};

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(mem_region* Region, size_t Size, size_t Align = 8){
    
    void* Result = 0;
    
    b32 NeedNewBlock = JOY_FALSE;
    
    size_t AlignedPos = 0;
    size_t NewUsedCount = 0;
    
    if(Region->Block){
        size_t BeforeAlign = (size_t)Region->Block->Block.Base + Region->Block->Block.Used;
        AlignedPos = (BeforeAlign + Align - 1) & (~(Align - 1));
        size_t AdvancedByAlign = AlignedPos - BeforeAlign;
        
        size_t ToAllocateSize = AdvancedByAlign + Size;
        NewUsedCount = Region->Block->Block.Used + ToAllocateSize;
        
        if(NewUsedCount > Region->Block->Block.Total){
            // NOTE(Dima): Need to allocate new block and set current
            // NOTE(Dima): block as old for new one.
            
            NeedNewBlock = JOY_TRUE;
            
            // NOTE(Dima): Finding empty but not used blocks
            while(Region->Block->Next != 0){
                if(Size <= Region->Block->Block.Total){
                    NeedNewBlock = JOY_FALSE;
                    
                    break;
                }
                
                Region->Block = Region->Block->Next;
            }
        }
    }
    else{
        // NOTE(Dima): Need to allocate new block and set 
        // NOTE(Dima): old and next as zeroes.
        
        NeedNewBlock = JOY_TRUE;
    }
    
    if(NeedNewBlock){
        // NOTE(Dima): Calculate this if we need to allocate new block
        size_t NewBlockSize;
        
        u32 MinRegionSize = Region->GrowSize;
        if(!MinRegionSize){
            MinRegionSize = MINIMUM_MEMORY_REGION_SIZE;
        }
        
        if(Size > MinRegionSize){
            NewBlockSize = Size;
        }
        else{
            NewBlockSize = MinRegionSize;
        }
        
        mem_block_entry* NewBlock = Platform.MemAlloc(NewBlockSize);
        NewBlock->Old = Region->Block; // NOTE(Dima): Region->Block is ZERO
        NewBlock->Next = 0;
        
        if(Region->Block){
            Region->Block->Next = NewBlock;
        }
        
        Region->Block = NewBlock;
        
        Result = Region->Block->Block.Base;
        Region->Block->Block.Used = Size;
    }
    else{
        // NOTE(Dima): Everything is OK. We can allocate here
        Assert(NewUsedCount <= Region->Block->Block.Total);
        
        Result = (void*)AlignedPos;
        Region->Block->Block.Used = NewUsedCount;
    }
    
    
    return(Result);
}

inline mem_block PushSplit(mem_region* Region, mi Size)
{
    mem_block Result;
    
    Result.Base = PushSomeMem(Region, Size);
    Result.Total = Size;
    Result.Used = 0;
    
    return(Result);
}

// NOTE(Dima): This function deallocates all region
inline void Free(mem_region* Region){
    while(Region->Block){
        mem_block_entry* OldBlock = Region->Block->Old;
        
        Platform.MemFree(Region->Block);
        
        Region->Block = OldBlock;
        
        if(Region->Block){
            Region->Block->Next = 0;
        }
    }
    
    Region->Block = 0;
}

// NOTE(Dima): This function frees all region but not deallocates 
// NOTE(Dima): its blocks but just zeroes them
inline void FreeNoDealloc(mem_region* Region){
    mem_block_entry* LastValid = Region->Block;
    
    while(Region->Block){
        Platform.MemZero(Region->Block);
        Region->Block->Block.Used = 0;
        
        LastValid = Region->Block;
        
        Region->Block = Region->Block->Old;
    }
    
    Region->Block = LastValid;
}

#define PushMemoryStruct(region, type, name, region_member_name) \
{\
    name = PushStruct(region, type);\
    name->##region_member_name = region;\
}

// NOTE(Dima): Definitions
#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

// NOTE(Dima): Memory box
mem_box InitMemoryBox(mem_region* Region, u32 BoxSizeInBytes);
mem_entry* AllocateMemoryFromBox(mem_box* box, u32 RequestMemorySize);
void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry);

// NOTE(Dima): Layered
void InitLayeredMem(layered_mem* Mem, 
                    mem_region* Region,
                    u32* LayersSizes,
                    int LayersSizesCount);

void DeallocateMemLayerEntry(layered_mem* Mem, 
                             mem_layer_entry* ToDeallocate);

mem_layer_entry* AllocateMemLayerEntry(layered_mem* MemLayered, 
                                       u32 SizeToAllocate);

#endif
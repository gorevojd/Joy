#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include "joy_platform.h"
#include <stdint.h>
#include <string.h>

// TODO(Dima): Add minimum block size param to allocation function

struct mem_region{
    Memory_Block Block;
    
    Memory_Block* Prev;
    
    int regionCount;
};

enum memory_entry_state{
    MemoryEntry_Released,
    MemoryEntry_Used,
};

struct mem_entry{
    mem_entry* NextAlloc;
    mem_entry* PrevAlloc;
    
    mem_entry* Next;
    mem_entry* Prev;
    
    
    void* Data;
    u32 DataSize;
    
    u32 State;
};

struct mem_box{
    mem_entry Use;
    mem_entry Free;
    
    mem_entry* First;
    
    mem_region* Region;
};

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(mem_region* region, size_t size, size_t align = 8){
    size_t beforeAlign = (size_t)region->Block.Base + region->Block.Used;
    size_t alignedPos = (beforeAlign + align - 1) & (~(align - 1));
    size_t advancedByAlign = alignedPos - beforeAlign;
    
    size_t toAllocateSize = advancedByAlign + size;
    size_t newUsedCount = region->Block.Used + toAllocateSize;
    
    void* result = 0;
    
    if((newUsedCount > region->Block.Total) || (region->Block.Base == 0))
    {
        // NOTE(Dima): Platform is guaranteed to allocate aligned mem
        // NOTE(Dima): so here i allocate only Size but not ToAllocateSize
        
        size_t sizeWithPrevBlock = size + sizeof(mem_region);
        size_t toAllocateWithPrevBlock;
        if(sizeWithPrevBlock > MINIMUM_MEMORY_REGION_SIZE){
            toAllocateWithPrevBlock = sizeWithPrevBlock;
        }
        else{
            toAllocateWithPrevBlock = MINIMUM_MEMORY_REGION_SIZE;
        }
        
        void* newBase = platform.MemAlloc(toAllocateWithPrevBlock);
        size_t newUsed = size;
        size_t newTotal = toAllocateWithPrevBlock - sizeof(mem_region);
        
        if(region->regionCount == 0){
            // NOTE(Dima): This is the first Block so it means it
            // NOTE(Dima): has not old region
        }
        else{
            mem_region* OldRegion = (mem_region*)((u8*)newBase + newTotal);
            OldRegion->Block.Base = region->Block.Base;
            OldRegion->Block.Used = region->Block.Used;
            OldRegion->Block.Total = region->Block.Total;
        }
        
        region->regionCount++;
        region->Block.Base = newBase;
        region->Block.Used = newUsed;
        region->Block.Total = newTotal;
        
        result = region->Block.Base;
    }
    else{
        Assert(newUsedCount <= region->Block.Total);
        
        result = (void*)alignedPos;
        region->Block.Used = newUsedCount;
    }
    
    return(result);
}

inline void FreeMemoryRegion(mem_region* region){
    while(region->regionCount > 0){
        mem_region* Old = (mem_region*)((u8*)region->Block.Base + region->Block.Total);
        
        platform.MemFree(region->Block.Base);
        
        region->Block.Base = Old->Block.Base;
        region->Block.Used = Old->Block.Used;
        region->Block.Total = Old->Block.Total;
        
        --region->regionCount;
    }
    
    ASSERT(region->regionCount == 0);
}

#define PushMemoryStruct(region, type, name, region_member_name) \
{\
    name = PushStruct(region, type);\
    name->##region_member_name = region;\
}

#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

mem_box InitMemoryBox(mem_region* Region, u32 BoxSizeInBytes);
mem_entry* AllocateMemoryFromBox(mem_box* box, u32 RequestMemorySize);
void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry);

#endif
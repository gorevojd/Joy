#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include "joy_platform.h"
#include <stdint.h>
#include <string.h>

enum MemoryRegionType{
    MemoryRegion_Dynamic,
    MemoryRegion_Static,
};

struct Memory_Region{
    Memory_Block block;
    
    int regionCount;
    
    u32 type;
};

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(Memory_Region* region, size_t size, size_t align = 8){
    size_t beforeAlign = (size_t)region->block.Base + region->block.Used;
    size_t alignedPos = (beforeAlign + align - 1) & (~(align - 1));
    size_t advancedByAlign = alignedPos - beforeAlign;
    
    size_t toAllocateSize = advancedByAlign + size;
    size_t newUsedCount = region->block.Used + toAllocateSize;
    
    void* result = 0;
    
    if(((newUsedCount > region->block.Total) || (region->block.Base == 0)) && 
       region->type == MemoryRegion_Dynamic)
    {
        // NOTE(Dima): Platform is guaranteed to allocate aligned mem
        // NOTE(Dima): so here i allocate only Size but not ToAllocateSize
        
        size_t sizeWithPrevBlock = size + sizeof(Memory_Region);
        size_t toAllocateWithPrevBlock;
        if(sizeWithPrevBlock > MINIMUM_MEMORY_REGION_SIZE){
            toAllocateWithPrevBlock = sizeWithPrevBlock;
        }
        else{
            toAllocateWithPrevBlock = MINIMUM_MEMORY_REGION_SIZE;
        }
        
        void* newBase = platform.MemAlloc(toAllocateWithPrevBlock);
        size_t newUsed = size;
        size_t newTotal = toAllocateWithPrevBlock - sizeof(Memory_Region);
        
        if(region->regionCount == 0){
            // NOTE(Dima): This is the first block so it means it
            // NOTE(Dima): has not old region
        }
        else{
            Memory_Region* OldRegion = (Memory_Region*)((u8*)newBase + newTotal);
            OldRegion->block.Base = region->block.Base;
            OldRegion->block.Used = region->block.Used;
            OldRegion->block.Total = region->block.Total;
            OldRegion->type = region->type;
        }
        
        region->regionCount++;
        region->block.Base = newBase;
        region->block.Used = newUsed;
        region->block.Total = newTotal;
        region->type = MemoryRegion_Dynamic;
        
        result = region->block.Base;
    }
    else{
        Assert(newUsedCount <= region->block.Total);
        
        result = (void*)alignedPos;
        region->block.Used = newUsedCount;
    }
    
    return(result);
}

inline void FreeMemoryRegion(Memory_Region* region){
    if(region->type == MemoryRegion_Dynamic){
        while(region->regionCount > 1){
            Memory_Region* old = (Memory_Region*)((u8*)region->block.Base + region->block.Total);
        }
    }
    else{
        
    }
}

// NOTE(Dima): Pushed region will can grow
inline Memory_Region PushMemoryRegionStatic(Memory_Region* existing, mi size){
    Memory_Region result = {};
    
    result.block.Base = PushSomeMem(existing, size);
    result.block.Total = size;
    result.block.Used = 0;
    result.type = MemoryRegion_Static;
    
    return(result);
}

inline Memory_Region PushMemoryRegionDynamic(Memory_Region* existing, mi size){
    Memory_Region result = {};
    
    result.block.Base = PushSomeMem(existing, size);
    result.block.Total = size;
    result.block.Used = 0;
    result.type = MemoryRegion_Dynamic;
    
    return(result);
}

#define PushMemoryStruct(region, type, name, region_member_name) \
{\
    type* asd = (type*)PushSomeMem(region, sizeof(type));\
    asd->##region_member_name = region;\
    name = asd;\
}

#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

#endif
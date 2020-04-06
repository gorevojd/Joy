#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include "joy_platform.h"
#include "joy_types.h"

#include <stdint.h>
#include <string.h>

struct mem_region{
    mem_block_entry* Block;
    
    b32 CreatedInsideBlock;
    mem_block CreationBlock;
    
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

inline void InitMemoryBlock(mem_block* Block, void* Base, mi Size){
    Block->Base = Base;
    Block->Used = 0;
    Block->Total = Size;
}

inline mem_block InitMemoryBlock(void* Base, mi Size){
    mem_block Block;
    
    Block.Base = Base;
    Block.Used = 0;
    Block.Total = Size;
    
    return(Block);
}

inline mi HowMuchLeft(mem_block* Block){
    mi Result = Block->Total - Block->Used;
    
    return(Result);
}

inline mi HowMuchLeft(mem_region* Region){
    
    mi Result = 0;
    
    if(Region->CreatedInsideBlock){
        Result = HowMuchLeft(&Region->CreationBlock);
    }
    else{
        if(Region->Block){
            Result = HowMuchLeft(&Region->Block->Block);
        }
        else{
            Result = 0;
        }
    }
    
    return(Result);
}

inline mi AdvancePointerToAlign(void* Address, int Align){
    mi BeforeAlign = (mi)Address;
    mi AlignedPos = (BeforeAlign + Align - 1) & (~(Align - 1));
    
    return(AlignedPos);
}

inline mem_region CreateInsideBlock(void* Base, mi Size){
    mem_region Result = {};
    
    Result.CreatedInsideBlock = true;
    
    Result.CreationBlock.Base = Base;
    Result.CreationBlock.Used = 0;
    Result.CreationBlock.Total = Size;
    
    return(Result);
}

inline mem_block* GetRegionBlock(mem_region* Region){
    mem_block* Block = 0;
    
    if(Region->CreatedInsideBlock){
        Block = &Region->CreationBlock;
    }
    else{
        if(Region->Block){
            Block = &Region->Block->Block;
        }
        else{
            // NOTE(Dima): Need to allocate new block and set 
            // NOTE(Dima): old and next as zeroes.
            
            Block = 0;
        }
    }
    
    return(Block);
}

inline void* GetRegionBase(mem_region* Region){
    mem_block* Block = GetRegionBlock(Region);
    
    void* Base = 0;
    if(Block){
        Base = Block->Base;
    }
    
    return(Base);
}

inline mem_region CreateInRestOfRegion(mem_region* Region, mi Align = 8){
    mi LeftInRegion = HowMuchLeft(Region);
    void* Base = GetRegionBase(Region);
    
    Assert(Base);
    
    mi NewBaseRaw = AdvancePointerToAlign(Base, Align);
    void* NewBase = (void*)NewBaseRaw;
    
    mi AdvancedByAlign = NewBaseRaw - (mi)Base;
    mi ActualLeft = LeftInRegion - AdvancedByAlign;
    
    Assert(ActualLeft > 0);
    
    mem_region Result = CreateInsideBlock(NewBase, ActualLeft);
    
    return(Result);
}

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(mem_region* Region, mi Size, mi Align = 8){
    void* Result = 0;
    
    b32 NeedNewBlock = false;
    mem_block* Block = GetRegionBlock(Region);
    
    mi InitBlockUsed = 0;
    mi InitBlockBase = 0;
    if(Block == 0){
        NeedNewBlock = true;
    }
    else{
        InitBlockUsed = Block->Used;
        InitBlockBase = (mi)Block->Base;
    }
    
    mi BeforeAlign = InitBlockBase + InitBlockUsed;
    mi AlignedPos = (BeforeAlign + Align - 1) & (~(Align - 1));
    mi AdvancedByAlign = AlignedPos - BeforeAlign;
    
    mi ToAllocateSize = AdvancedByAlign + Size;
    mi NewUsedCount = InitBlockUsed + ToAllocateSize;
    
    if((Region->CreatedInsideBlock == false) && Region->Block){
        if(NewUsedCount > Region->Block->Block.Total){
            // NOTE(Dima): Need to allocate new block and set current
            // NOTE(Dima): block as old for new one.
            
            NeedNewBlock = true;
            
            // NOTE(Dima): Finding empty but not used blocks
            while(Region->Block->Next != 0){
                if(Size <= Region->Block->Block.Total){
                    NeedNewBlock = false;
                    
                    break;
                }
                
                Region->Block = Region->Block->Next;
            }
        }
    }
    
    if(NeedNewBlock){
        Assert(Region->CreatedInsideBlock == false);
        
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
        Assert(NewUsedCount <= Block->Total);
        
        Result = (void*)AlignedPos;
        Block->Used = NewUsedCount;
    }
    
    return(Result);
}

// NOTE(Dima): This function deallocates all region
inline void Free(mem_region* Region){
    if(Region->CreatedInsideBlock){
        Region->CreationBlock.Used = 0;
    }
    else{
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
}

// NOTE(Dima): This function frees all region but not deallocates 
// NOTE(Dima): its blocks but just zeroes them
inline void FreeNoDealloc(mem_region* Region){
    if(Region->CreatedInsideBlock){
        Region->CreationBlock.Used = 0;
    }
    else{
        mem_block_entry* LastValid = Region->Block;
        
        while(Region->Block){
            Platform.MemZero(Region->Block);
            Region->Block->Block.Used = 0;
            
            LastValid = Region->Block;
            
            Region->Block = Region->Block->Old;
        }
        
        Region->Block = LastValid;
    }
}

inline mem_region PushSplit(mem_region* Region, mi Size)
{
    void* ResultBase = PushSomeMem(Region, Size);
    
    mem_region Result = CreateInsideBlock(ResultBase, Size);
    
    return(Result);
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
#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include "joy_platform.h"
#include "joy_types.h"
#include <stdint.h>
#include <string.h>

// TODO(Dima): Add minimum block size param to allocation function

struct mem_region{
    mem_block* Block;
};

enum memory_entry_state{
    MemoryEntry_Released,
    MemoryEntry_Used,
};

struct mem_entry_snapshot{
    u32 DataSize;
    
    struct mem_entry* CorrespondingEntry;
    
    b32 operator>(mem_entry_snapshot& Entry){
        b32 Result = this->DataSize > Entry.DataSize;
        
        return(Result);
    }
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
    
    mem_entry_snapshot* Snapshot;
};

struct mem_box{
    mem_region* Region;
    mem_entry Use;
    mem_entry Free;
    
    mem_entry* First;
    mem_entry* FirstReleased;
    
    std::vector<mem_entry_snapshot> ReleasedSnapshots;
    
    mem_box* NextBox;
};

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(mem_region* Region, size_t Size, size_t Align = 8){
    
    void* Result = 0;
    
    b32 NeedNewBlock = JOY_FALSE;
    
    size_t AlignedPos = 0;
    size_t NewUsedCount = 0;
    
    if(Region->Block){
        size_t BeforeAlign = (size_t)Region->Block->Base + Region->Block->Used;
        AlignedPos = (BeforeAlign + Align - 1) & (~(Align - 1));
        size_t AdvancedByAlign = AlignedPos - BeforeAlign;
        
        size_t ToAllocateSize = AdvancedByAlign + Size;
        NewUsedCount = Region->Block->Used + ToAllocateSize;
        
        if(NewUsedCount > Region->Block->Total){
            // NOTE(Dima): Need to allocate new block and set current
            // NOTE(Dima): block as old for new one.
            
            NeedNewBlock = JOY_TRUE;
            
            while(Region->Block->Next != 0){
                if(Size <= Region->Block->Total){
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
        if(Size > MINIMUM_MEMORY_REGION_SIZE){
            NewBlockSize = Size;
        }
        else{
            NewBlockSize = MINIMUM_MEMORY_REGION_SIZE;
        }
        
        mem_block* NewBlock = platform.MemAlloc(NewBlockSize);
        NewBlock->Old = Region->Block; // NOTE(Dima): Region->Block is ZERO
        NewBlock->Next = 0;
        
        if(Region->Block){
            Region->Block->Next = NewBlock;
        }
        
        Region->Block = NewBlock;
        
        Result = Region->Block->Base;
        Region->Block->Used = Size;
    }
    else{
        // NOTE(Dima): Everything is OK. We can allocate here
        Assert(NewUsedCount <= Region->Block->Total);
        
        Result = (void*)AlignedPos;
        Region->Block->Used = NewUsedCount;
    }
    
    
    return(Result);
}

// NOTE(Dima): This function deallocates all region
inline void Free(mem_region* Region){
    while(Region->Block){
        mem_block* OldBlock = Region->Block->Old;
        
        platform.MemFree(Region->Block);
        
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
    mem_block* LastValid = Region->Block;
    
    while(Region->Block){
        platform.MemZero(Region->Block);
        Region->Block->Used = 0;
        
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

#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

mem_box InitMemoryBox(mem_region* Region, u32 BoxSizeInBytes);
mem_entry* AllocateMemoryFromBox(mem_box* box, u32 RequestMemorySize);
void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry);

#endif
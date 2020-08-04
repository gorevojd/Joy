#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

struct mem_arena{
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
    mem_arena* Arena;
    
    mem_layer_entry* LayersSentinels;
    mem_layer_entry* FreeSentinels;
    int LayersCount;
};

inline void RegionSetGrowSize(mem_arena* Arena, u32 GrowSize){
    Arena->GrowSize = GrowSize;
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
    mem_arena* Arena;
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

inline mi HowMuchLeft(mem_arena* Arena){
    
    mi Result = 0;
    
    if(Arena->CreatedInsideBlock){
        Result = HowMuchLeft(&Arena->CreationBlock);
    }
    else{
        if(Arena->Block){
            Result = HowMuchLeft(&Arena->Block->Block);
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

inline mem_arena CreateInsideBlock(void* Base, mi Size){
    mem_arena Result = {};
    
    Result.CreatedInsideBlock = true;
    
    Result.CreationBlock.Base = Base;
    Result.CreationBlock.Used = 0;
    Result.CreationBlock.Total = Size;
    
    return(Result);
}

inline mem_block* GetRegionBlock(mem_arena* Arena){
    mem_block* Block = 0;
    
    if(Arena->CreatedInsideBlock){
        Block = &Arena->CreationBlock;
    }
    else{
        if(Arena->Block){
            Block = &Arena->Block->Block;
        }
        else{
            // NOTE(Dima): Need to allocate new block and set 
            // NOTE(Dima): old and next as zeroes.
            
            Block = 0;
        }
    }
    
    return(Block);
}

inline void* GetRegionBase(mem_arena* Arena){
    mem_block* Block = GetRegionBlock(Arena);
    
    void* Base = 0;
    if(Block){
        Base = Block->Base;
    }
    
    return(Base);
}

inline mem_arena CreateInRestOfRegion(mem_arena* Arena, mi Align = 8){
    mi LeftInRegion = HowMuchLeft(Arena);
    void* Base = GetRegionBase(Arena);
    
    Assert(Base);
    
    mi NewBaseRaw = AdvancePointerToAlign(Base, Align);
    void* NewBase = (void*)NewBaseRaw;
    
    mi AdvancedByAlign = NewBaseRaw - (mi)Base;
    mi ActualLeft = LeftInRegion - AdvancedByAlign;
    
    Assert(ActualLeft > 0);
    
    mem_arena Result = CreateInsideBlock(NewBase, ActualLeft);
    
    return(Result);
}

#define MINIMUM_MEMORY_REGION_SIZE Mibibytes(1)
inline void* PushSomeMem(mem_arena* Arena, mi Size, mi Align = 8){
    void* Result = 0;
    
    b32 NeedNewBlock = false;
    mem_block* Block = GetRegionBlock(Arena);
    
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
    
    if((Arena->CreatedInsideBlock == false) && Arena->Block){
        if(NewUsedCount > Arena->Block->Block.Total){
            // NOTE(Dima): Need to allocate new block and set current
            // NOTE(Dima): block as old for new one.
            
            NeedNewBlock = true;
            
            // NOTE(Dima): Finding empty but not used blocks
            while(Arena->Block->Next != 0){
                if(Size <= Arena->Block->Block.Total){
                    NeedNewBlock = false;
                    
                    break;
                }
                
                Arena->Block = Arena->Block->Next;
            }
        }
    }
    
    if(NeedNewBlock){
        Assert(Arena->CreatedInsideBlock == false);
        
        // NOTE(Dima): Calculate this if we need to allocate new block
        size_t NewBlockSize;
        
        u32 MinRegionSize = Arena->GrowSize;
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
        NewBlock->Old = Arena->Block; // NOTE(Dima): Arena->Block is ZERO
        NewBlock->Next = 0;
        
        if(Arena->Block){
            Arena->Block->Next = NewBlock;
        }
        
        Arena->Block = NewBlock;
        
        Result = Arena->Block->Block.Base;
        Arena->Block->Block.Used = Size;
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
inline void Free(mem_arena* Arena){
    if(Arena->CreatedInsideBlock){
        Arena->CreationBlock.Used = 0;
    }
    else{
        while(Arena->Block){
            mem_block_entry* OldBlock = Arena->Block->Old;
            
            Platform.MemFree(Arena->Block);
            
            Arena->Block = OldBlock;
            
            if(Arena->Block){
                Arena->Block->Next = 0;
            }
        }
        
        Arena->Block = 0;
    }
}

// NOTE(Dima): This function frees all region but not deallocates 
// NOTE(Dima): its blocks but just zeroes them
inline void FreeNoDealloc(mem_arena* Arena){
    if(Arena->CreatedInsideBlock){
        Arena->CreationBlock.Used = 0;
    }
    else{
        mem_block_entry* LastValid = Arena->Block;
        
        while(Arena->Block){
            Platform.MemZero(Arena->Block);
            Arena->Block->Block.Used = 0;
            
            LastValid = Arena->Block;
            
            Arena->Block = Arena->Block->Old;
        }
        
        Arena->Block = LastValid;
    }
}

inline mem_arena PushSplit(mem_arena* Arena, mi Size)
{
    void* ResultBase = PushSomeMem(Arena, Size);
    
    mem_arena Result = CreateInsideBlock(ResultBase, Size);
    
    return(Result);
}

inline void* PushInMemoryStruct_(mem_arena* Arena, mi SizeOfType, mi OffsetOfArenaMember)
{
    void* Result = PushSomeMem(Arena, SizeOfType);
    mi* Ptr = (mi*)((u8*)Result + OffsetOfArenaMember);
    *(Ptr) = (mi)Arena;
    
    return(Result);
}

#define PushInMemoryStruct(arena, type, arena_member_name) (type*)PushInMemoryStruct_(arena, sizeof(type), offsetof(type, arena_member_name))

#if 0
#define PushMemoryStruct(region, type, name, region_member_name) \
{\
name = PushStruct(region, type);\
name->##region_member_name = region;\
}
#endif

// NOTE(Dima): Definitions
#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

// NOTE(Dima): Memory box
mem_box InitMemoryBox(mem_arena* Arena, u32 BoxSizeInBytes);
mem_entry* AllocateMemoryFromBox(mem_box* box, u32 RequestMemorySize);
void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry);

// NOTE(Dima): Layered
void InitLayeredMem(layered_mem* Mem, 
                    mem_arena* Arena,
                    u32* LayersSizes,
                    int LayersSizesCount);

void DeallocateMemLayerEntry(layered_mem* Mem, 
                             mem_layer_entry* ToDeallocate);

mem_layer_entry* AllocateMemLayerEntry(layered_mem* MemLayered, 
                                       u32 SizeToAllocate);

#endif
#include "joy_memory.h"

INTERNAL_FUNCTION void InitLayerSentinels(mem_layer_entry* Entries, 
                                          u32* LayersSizes,
                                          int LayersSizesCount)
{
    for(int SentIndex = 0;
        SentIndex < LayersSizesCount;
        SentIndex++)
    {
        mem_layer_entry* LayerEntry = Entries + SentIndex;
        
        LayerEntry->Next = LayerEntry;
        LayerEntry->Prev = LayerEntry;
        
        LayerEntry->LayerSize = LayersSizes[SentIndex];
        LayerEntry->Data = 0;
    }
}

void InitLayeredMem(layered_mem* Mem, 
                    mem_arena* Arena,
                    u32* LayersSizes,
                    int LayersSizesCount)
{
    *Mem = {};
    
    Mem->Arena = Arena;;
    Mem->LayersSentinels = PushArray(Arena, mem_layer_entry, LayersSizesCount);
    Mem->FreeSentinels = PushArray(Arena, mem_layer_entry, LayersSizesCount);
    Mem->LayersCount = LayersSizesCount;
    
    
    // NOTE(Dima): Initializing layers sentinels
    InitLayerSentinels(Mem->LayersSentinels, LayersSizes, LayersSizesCount);
    
    // NOTE(Dima): Initializing layers free sentinels
    InitLayerSentinels(Mem->FreeSentinels, LayersSizes, LayersSizesCount);
}

void DeallocateMemLayerEntry(layered_mem* Mem, mem_layer_entry* ToDeallocate){
    int LayerIndex = ToDeallocate->LayerIndex;
    
    mem_layer_entry* Sent = &Mem->LayersSentinels[LayerIndex];
    mem_layer_entry* Free = &Mem->FreeSentinels[LayerIndex];
    
    DLIST_REMOVE_ENTRY(ToDeallocate, Next, Prev);
    DLIST_INSERT_BEFORE(ToDeallocate, Free, Next, Prev);
}

mem_layer_entry* 
AllocateMemLayerEntry(layered_mem* MemLayered, u32 SizeToAllocate)
{
    int ResultLayerIndex = -1;
    u32 ResultLayerSize = 0xFFFFFFFF;
    
    // NOTE(Dima): Detecting best fit layer
    for(int LayerIndex = 0;
        LayerIndex < MemLayered->LayersCount;
        LayerIndex++)
    {
        u32 LayerSize = MemLayered->LayersSentinels[LayerIndex].LayerSize;
        
        if((LayerSize > SizeToAllocate) && 
           (LayerSize < ResultLayerSize))
        {
            ResultLayerIndex = LayerIndex;
            ResultLayerSize = LayerSize;
        }
        
        ASSERT(ResultLayerSize >= SizeToAllocate);
    }
    
    mem_layer_entry* Result = 0;
    
    if(ResultLayerIndex != -1){
        mem_layer_entry* Sent = &MemLayered->LayersSentinels[ResultLayerIndex];
        mem_layer_entry* Free = &MemLayered->FreeSentinels[ResultLayerIndex];
        
        if(Free->Next == Free){
            const int Count = 128;
            mem_layer_entry* Pool = PushArray(MemLayered->Arena, mem_layer_entry, Count);
            
            for(int I = 0; I < Count; I++){
                mem_layer_entry* Elem = &Pool[I];
                
                Elem->LayerSize = ResultLayerSize;
                Elem->Data = PushSomeMem(MemLayered->Arena, ResultLayerSize);
                Elem->LayerIndex = ResultLayerIndex;
                
                DLIST_INSERT_BEFORE(Elem, Free, Next, Prev);
            }
        }
        
        Result = Free->Next;
        
        DLIST_REMOVE_ENTRY(Result, Next, Prev);
        DLIST_INSERT_BEFORE(Result, Sent, Next, Prev);
    }
    
    return(Result);
}

INTERNAL_FUNCTION inline mem_entry* AllocateMemoryEntry(mem_box* box) {
	mem_entry* Result = 0;
    
	if (box->Free.NextAlloc == &box->Free) 
	{
		/*
   NOTE(dima): I don't want memory entries to have
   bad order in memory. So i will allocate them by
   arrays and insert to freelist
  */
        
		//NOTE(dima): Allocating new entries array
		int NewEntriesCount = 1200;
		mem_entry* NewEntriesArray = PushArray(
                                               box->Arena,
                                               mem_entry,
                                               NewEntriesCount);
        
		//NOTE(dima): Inserting new entries to freelist
		for (int EntryIndex = 0;
             EntryIndex < NewEntriesCount;
             EntryIndex++)
		{
			mem_entry* CurrentEntry = NewEntriesArray + EntryIndex;
            
			*CurrentEntry = {};
            
			CurrentEntry->NextAlloc = box->Free.NextAlloc;
			CurrentEntry->PrevAlloc = &box->Free;
            
			CurrentEntry->NextAlloc->PrevAlloc = CurrentEntry;
			CurrentEntry->PrevAlloc->NextAlloc = CurrentEntry;
		}
	}
    
	//NOTE(dima): Allocating entry
	Result = box->Free.NextAlloc;
    
	Result->NextAlloc->PrevAlloc = Result->PrevAlloc;
	Result->PrevAlloc->NextAlloc = Result->NextAlloc;
    
	Result->NextAlloc = box->Use.NextAlloc;
	Result->PrevAlloc = &box->Use;
    
	Result->NextAlloc->PrevAlloc = Result;
	Result->PrevAlloc->NextAlloc = Result;
    
	Result->_InternalData = 0;
	Result->_InternalDataSize = 0;
	Result->State = MemoryEntry_Released;
    
	return(Result);
}

INTERNAL_FUNCTION inline void DeallocateMemoryEntry(
                                                    mem_box* box,
                                                    mem_entry* to)
{
	to->NextAlloc->PrevAlloc = to->PrevAlloc;
	to->PrevAlloc->NextAlloc = to->NextAlloc;
    
	to->NextAlloc = box->Free.NextAlloc;
	to->PrevAlloc = &box->Free;
    
	to->PrevAlloc->NextAlloc = to;
	to->NextAlloc->PrevAlloc = to;
}


INTERNAL_FUNCTION inline mem_entry* PopFromReleasedList(mem_entry* memEntry){
    if(memEntry->PrevReleased){
        memEntry->PrevReleased->NextReleased = memEntry->NextReleased;
    }
    
    if(memEntry->NextReleased){
        memEntry->NextReleased->PrevReleased = memEntry->PrevReleased;
    }
    
    memEntry->PrevReleased = 0;
    memEntry->NextReleased = 0;
    
    return(memEntry);
}

INTERNAL_FUNCTION inline mem_entry* PushToReleasedList(mem_box* Box, mem_entry* memEntry){
    memEntry->PrevReleased = 0;
    memEntry->NextReleased = Box->FirstReleased;
    
    if(memEntry->NextReleased){
        memEntry->NextReleased->PrevReleased = memEntry;
    }
    
    return(memEntry);
}


struct split_entries_result{
    mem_entry* First;
    mem_entry* Second;
};

/*
 NOTE(dima): This function splits memory entry into 
 2 parts and then returns first splited part with the
 requested memory size
*/
INTERNAL_FUNCTION split_entries_result SplitMemoryEntry(
                                                        mem_box* box,
                                                        mem_entry* toSplit,
                                                        u32 SplitOffset)
{
	/*
  NOTE(dima): If equal then second block will be 0 bytes,
  so we dont need equal and the sign is <
 */
	ASSERT(SplitOffset <= toSplit->_InternalDataSize);
    
	//NOTE(dima): allocating entries
	mem_entry* NewEntry1 = AllocateMemoryEntry(box);
	mem_entry* NewEntry2 = AllocateMemoryEntry(box);
    
	NewEntry1->Prev = toSplit->Prev;
	NewEntry1->Next = NewEntry2;
    
	NewEntry2->Prev = NewEntry1;
	NewEntry2->Next = toSplit->Next;
    
	if (toSplit->Prev) {
		toSplit->Prev->Next = NewEntry1;
	}
    
	if (toSplit->Next) {
		toSplit->Next->Prev = NewEntry2;
	}
    
	//NOTE(dima): setting entries data
	NewEntry1->_InternalDataSize = SplitOffset;
	NewEntry1->_InternalData = toSplit->_InternalData;
    
	NewEntry2->_InternalDataSize = toSplit->_InternalDataSize - SplitOffset;
	NewEntry2->_InternalData = (void*)((u8*)toSplit->_InternalData + SplitOffset);
    
	//NOTE(dima): Deallocating initial entry
	DeallocateMemoryEntry(box, toSplit);
    
	ASSERT(NewEntry1->_InternalDataSize + NewEntry2->_InternalDataSize == toSplit->_InternalDataSize);
    
    split_entries_result Result = {};
    Result.First = NewEntry1;
    Result.Second = NewEntry2;
    
	return(Result);
}

/*
 NOTE(dima): This function merges 2 memory entries.
 Return value is equal to the First parameter and 
 contatains merged block.
*/
INTERNAL_FUNCTION mem_entry* MergeMemoryEntries(
                                                mem_box* box, 
                                                mem_entry* First,
                                                mem_entry* Second) 
{
	Assert(First->Next == Second);
    
	First->Next = Second->Next;
    
	First->_InternalDataSize = First->_InternalDataSize + Second->_InternalDataSize;
    
	DeallocateMemoryEntry(box, Second);
    
	mem_entry* Result = First;
	return(Result);
}

mem_entry* AllocateMemoryFromBox(
                                 mem_box* box,
                                 u32 RequestMemorySize,
                                 u32 Align)
{
	mem_entry* Result = 0;
    
    // NOTE(Dima): We should do this to ensure if we actually can
    // NOTE(Dima): hold new block with alignment
    u32 RequestMemorySizeWithAlign = RequestMemorySize + Align;
    
    /*
NOTE(dima): Merge loop.
In this loop i walk through all allocated memory entries
and try to merge those that lie near each other
*/
    int TempCounter = 0;
	mem_entry* At = box->FirstReleased;
    
	while (At) {
		mem_entry* NextAt = At->NextReleased;
        
        ASSERT(At->State == MemoryEntry_Released);
        
        while (NextAt) {
            if (NextAt->State == MemoryEntry_Released) {
                PopFromReleasedList(NextAt);
                
                At = MergeMemoryEntries(box, At, NextAt);
                NextAt = At->Next;
            }
            else {
                break;
            }
		}
        
		TempCounter++;
		At = NextAt;
	}
    
#if 0
	//NOTE(dima): Find loop
	At = box->First;
	while (At) {
		if (At->State == MemoryEntry_Released &&  
			At->_InternalDataSize >= RequestMemorySize) 
		{
            // NOTE(Dima): Slit only when larger
            if(At->_InternalDataSize > RequestMemorySizeWithAlign){
                split_entries_result SlitRes = SplitMemoryEntry(box, At, RequestMemorySizeWithAlign);
                Result = SplitRes.First;
                
                PushToReleasedList(SplitRes.Second);
            }
            else{
                Result = At;
            }
            
            break;
        }
        
        At = At->Next;
    }
#else
    At = box->FirstReleased;
    
    
#endif
    
    if (Result) {
        Result->State = MemoryEntry_Used;
        
        size_t BeforeAlign = (size_t)Result->_InternalData;
        size_t AlignedPos = (BeforeAlign + Align - 1) & (~((size_t)Align - 1));
        size_t AdvancedByAlign = AlignedPos - BeforeAlign;
        
        Result->ActualData = (void*)AlignedPos;
        Result->ActualDataSize = AdvancedByAlign + RequestMemorySize;
        
        Assert(RequestMemorySize == Result->_InternalDataSize);
        Assert(Result->State == MemoryEntry_Used);
    }
    
    return(Result);
}

void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry) {
    
    memEntry->State = MemoryEntry_Released;
    
    PushToReleasedList(box, memEntry);
}

mem_box InitMemoryBox(mem_arena* Arena, u32 BoxSizeInBytes){
    
    mem_box Result = {};
    
    // NOTE(Dima): Memory initialization
    Result.Free = {};
    Result.Free.Next = &Result.Free;
    Result.Free.Prev = &Result.Free;
    
    Result.Use = {};
    Result.Use.Next = &Result.Use;
    Result.Use.Prev = &Result.Use;
    
    Result.Arena = Arena;
    
    Result.First = AllocateMemoryEntry(&Result);
    Result.First->Next = 0;
    Result.First->Prev = 0;
    Result.First->NextReleased = 0;
    Result.First->PrevReleased = 0;
    Result.First->_InternalData = PushSomeMem(Arena, BoxSizeInBytes, 16);
    Result.First->_InternalDataSize = BoxSizeInBytes;
    Result.First->State = MemoryEntry_Released;
    Result.FirstReleased = Result.First;
    
    Result.NextBox = 0;
    
    return(Result);
}

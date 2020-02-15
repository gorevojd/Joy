#include "joy_memory.h"


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
			box->Region,
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
    
	Result->Data = 0;
	Result->DataSize = 0;
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


/*
 NOTE(dima): This function splits memory entry into 
 2 parts and then returns first splited part with the
 requested memory size
*/
INTERNAL_FUNCTION mem_entry* SplitMemoryEntry(
mem_box* box,
mem_entry* toSplit,
u32 SplitOffset)
{
	/*
  NOTE(dima): If equal then second block will be 0 bytes,
  so we dont need equal and the sign is <
 */
	ASSERT(SplitOffset <= toSplit->DataSize);
    
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
	NewEntry1->DataSize = SplitOffset;
	NewEntry1->Data = toSplit->Data;
    
	NewEntry2->DataSize = toSplit->DataSize - SplitOffset;
	NewEntry2->Data = (void*)((u8*)toSplit->Data + SplitOffset);
    
	//NOTE(dima): Deallocating initial entry
	DeallocateMemoryEntry(box, toSplit);
    
	ASSERT(NewEntry1->DataSize + NewEntry2->DataSize == toSplit->DataSize);
    
	return(NewEntry1);
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
    
	First->DataSize = First->DataSize + Second->DataSize;
    
	DeallocateMemoryEntry(box, Second);
    
	mem_entry* Result = First;
	return(Result);
}

mem_entry* AllocateMemoryFromBox(
mem_box* box,
u32 RequestMemorySize)
{
	mem_entry* Result = 0;
    
	/*
  NOTE(dima): Merge loop.
  In this loop i walk through all allocated memory entries
  and try to merge those that lie near each other
 */
	int TempCounter = 0;
	mem_entry* At = box->First;
    
	while (At) {
		mem_entry* NextAt = At->Next;
        
		if (At->State == MemoryEntry_Released) {
            
			while (NextAt) {
				if (NextAt->State == MemoryEntry_Released) {
					At = MergeMemoryEntries(box, At, NextAt);
					NextAt = At->Next;
				}
				else {
					break;
				}
			}
		}
        
		TempCounter++;
		At = NextAt;
	}
    
	//NOTE(dima): Find loop
	At = box->First;
	while (At) {
		if (At->State == MemoryEntry_Released &&  
			At->DataSize >= RequestMemorySize) 
		{
            // NOTE(Dima): Slit only when larger
            if(At->DataSize > RequestMemorySize){
                Result = SplitMemoryEntry(box, At, RequestMemorySize);
            }
            else{
                Result = At;
            }
            
            break;
        }
        
        At = At->Next;
    }
    
    
    if (Result) {
        Result->State = MemoryEntry_Used;
        
        Assert(RequestMemorySize == Result->DataSize);
        Assert(Result->State == MemoryEntry_Used);
    }
    
    return(Result);
}

void ReleaseMemoryFromBox(mem_box* box, mem_entry* memEntry) {
    memEntry->State = MemoryEntry_Released;
}

mem_box InitMemoryBox(mem_region* Region, u32 BoxSizeInBytes){
    
    mem_box Result = {};
    
    // NOTE(Dima): Memory initialization
    Result.Free = {};
    Result.Free.Next = &Result.Free;
    Result.Free.Prev = &Result.Free;
    
    Result.Use = {};
    Result.Use.Next = &Result.Use;
    Result.Use.Prev = &Result.Use;
    
    Result.Region = Region;
    
    Result.First = AllocateMemoryEntry(&Result);
    Result.First->Next = 0;
    Result.First->Prev = 0;
    Result.First->Data = PushSomeMem(Region, BoxSizeInBytes, 16);
    Result.First->DataSize = BoxSizeInBytes;
    
    Result.NextBox = 0;
    
    return(Result);
}

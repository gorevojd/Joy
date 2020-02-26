#ifndef JOY_DATA_STRUCTURES_H
#define JOY_DATA_STRUCTURES_H

#include "joy_types.h"
#include "joy_memory.h"

template <typename t> struct dlist_entry{
    t Type;
    
    int DEBUGIndex;
    
    dlist_entry<t>* Prev;
    dlist_entry<t>* Next;
    
    void ReflectPointers(){
        this->Next = this;
        this->Prev = this;
    }
    
    void InsertBeforeListEntry(dlist_entry<t>* A){
        this->Next = A;
        this->Prev = A->Prev;
        
        this->Next->Prev = this;
        this->Prev->Next = this;
    }
    
    void InsertAfterListEntry(dlist_entry<t>* A){
        this->Next = A->Next;
        this->Prev = A;
        
        this->Next->Prev = this;
        this->Prev->Next = this;
    }
};

template <typename t> struct dlist{
    dlist_entry<t> Use;
    dlist_entry<t> Free;
    
    u32 Capacity;
    u32 Count;
    
    mem_region* Mem;
    
    void Init(mem_region* Memory){
        this->Capacity = 0;
        this->Count = 0;
        
        this->Mem = Memory;
        
        this->Free.DEBUGIndex = -1;
        this->Use.DEBUGIndex = -1;
        
        this->Free.ReflectPointers();
        this->Use.ReflectPointers();
    }
    
    b32 IsEmpty(){
        b32 Empty = &Use == Use.Next;
        
        return(Result);
    }
    
    void Clear(){
        DeallocateAll();
    }
    
    dlist_entry<t>* AllocateElement(b32 IsPushBack = 0){
        dlist_entry<t>* Result = 0;
        
        if(Free.Next == &Free){
            const int ToAllocateCount = 128;
            
            dlist_entry<t> *Allocated = PushArray(Mem, dlist_entry<t>, ToAllocateCount);
            
            for(int NewIndex = 0; NewIndex < ToAllocateCount; NewIndex++){
                dlist_entry<t>* New = &Allocated[NewIndex];
                
                New->DEBUGIndex = NewIndex;
                
                // NOTE(Dima): Inserting to free list
                New->Next = Free.Next;
                New->Prev = &Free;
                
                New->Next->Prev = New;
                New->Prev->Next = New;
            }
            
            Capacity += ToAllocateCount;
        }
        
        Result = Free.Next;
        
        // NOTE(Dima): Removing from free list
        Result->Next->Prev = Result->Prev;
        Result->Prev->Next = Result->Next;
        
        // NOTE(Dima): Inserting to Use list
        if(IsPushBack){
            Result->Next = &Use;
            Result->Prev = Use.Prev;
        }
        else{
            Result->Next = Use.Next;
            Result->Prev = &Use;
        }
        
        Result->Next->Prev = Result;
        Result->Prev->Next = Result;
        
        // NOTE(Dima): Incrementing count
        ++Count;
        
        return(Result);
    }
    
    t* Alloc(){
        dlist_entry<t>* Allocated = AllocateElement(JOY_TRUE);
        t* Result = &Allocated->Type;
        
        return(Result);
    }
    
    
    void DeallocateAll(){
        // NOTE(Dima): If list is not empty
        if(Use.Next != &Use){
            
            dlist_entry<t>* First = Use.Next;
            dlist_entry<t>* Last = Use.Prev;
            
            // NOTE(Dima): If it's more than one element
            if(First != Last){
#if 1
                Use.ReflectPointers();
#else
                First->Prev->Next = Last->Next;
                First->Next->Prev = Last->Prev;
#endif
                
                // NOTE(Dima): Deleting all elements by resetting first's and last's pointers
                First->Prev = &Free;
                Last->Next = Free.Next;
                
                First->Prev->Next = First;
                Last->Next->Prev = Last;
                
                // NOTE(Dima): Resetting count
                
            }
            else{
                dlist_entry<t>* Entry = First;
                
                // NOTE(Dima): Deleting from Use list
                Entry->Next->Prev = Entry->Prev;
                Entry->Prev->Next = Entry->Next;
                
                // NOTE(Dima): Inserting to Free list
                Entry->Next = Free.Next;
                Entry->Prev = &Free;
                
                Entry->Next->Prev = Entry;
                Entry->Prev->Next = Entry;
            }
        }
        
        Count = 0;
    }
    
    void DeallocateElement(dlist_entry<t>* Entry){
        // NOTE(Dima): Deleting from Use list
        Entry->Next->Prev = Entry->Prev;
        Entry->Prev->Next = Entry->Next;
        
        // NOTE(Dima): Inserting to Free list
        Entry->Next = Free.Next;
        Entry->Prev = &Free;
        
        Entry->Next->Prev = Entry;
        Entry->Prev->Next = Entry;
        
        // NOTE(Dima): Decreasing count
        --Count;
    }
    
    void Push(const t& Type){
        dlist_entry<t>* Entry = AllocateElement();
        
        Entry->Type = Type;
    }
};

#endif
#ifndef JOY_MEMORY_H
#define JOY_MEMORY_H

#include "joy_defines.h"
#include <stdint.h>
#include <string.h>

struct memory_region{
    void* Base;
    size_t Used;
    size_t Total;
};

inline memory_region InitMemoryRegion(void* Base, size_t Size){
    memory_region Result = {};
    
    Result.Base = Base;
    Result.Total = Size;
    Result.Used = 0;
    
    return(Result);
}

inline void* PushSomeMem(memory_region* Region, size_t Size, size_t Align = 8){
    
    size_t BeforeAlign = (size_t)Region->Base + Region->Used;
    size_t AlignedPos = (BeforeAlign + Align - 1) & (~(Align - 1));
    size_t AdvancedByAlign = AlignedPos - BeforeAlign;
    
    size_t NewUsedCount = Region->Used + AdvancedByAlign + Size;
    Assert(NewUsedCount <= Region->Total);
    
    void* Result = (void*)AlignedPos;
    
    Region->Used = NewUsedCount;
    
    int Temp = (size_t)Result & (Align - 1);
    Assert(Temp == 0);
    
    return(Result);
}

inline memory_region SplitMemoryRegion(memory_region* Region, size_t NewStackSize){
    memory_region Result = {};
    
    Result.Base = PushSomeMem(Region, NewStackSize);
    Result.Total = NewStackSize;
    Result.Used = 0;
    
    return(Result);
}

#define PushSize(region, size) PushSomeMem(region, size)
#define PushStruct(region, type) (type*)PushSomeMem(region, sizeof(type))
#define PushArray(region, type, count) (type*)PushSomeMem(region, sizeof(type) * count)
#define PushString(region, text) (char*)PushSomeMem(region, strlen(text) + 1)
#define PushStringSize(region, text, textlen) (char*)PushSomeMem(region, (textlen) + 1)

#endif
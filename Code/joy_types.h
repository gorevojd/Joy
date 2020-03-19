#ifndef JOY_TYPES_H
#define JOY_TYPES_H

#include <stdint.h>

typedef int b32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed char i8;
typedef signed short s16;
typedef signed short i16;
typedef signed int s32;
typedef signed int i32;
typedef signed long long s64;
typedef signed long long i64;

typedef float f32;
typedef double f64;

typedef size_t mi;
typedef intptr_t imi;
typedef uintptr_t umi;

typedef u16 utf16_t;
typedef u8 utf8_t;

struct mem_block{
    void* Base;
    size_t Used;
    size_t Total;
};

struct mem_block_entry{
    mem_block Block;
    
    mem_block_entry* Old;
    mem_block_entry* Next;
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

inline int SafeTruncateToUInt(float Value){
    int Result = (int)(Value + 0.5f);
    
    return(Result);
}

inline int SafeTruncateToInt(float Value){
    int Result;
    
    if(Value > 0.0f){
        Result = (int)(Value + 0.5f);
    }
    else{
        Result = (int)(Value - 0.5f);
    }
    
    return(Result);
}

inline void ToggleBool(b32* Value){
    *Value = !*Value;
}

inline void CopyFloats(f32* Dst, f32* Src, int Count){
    for(int i = 0; i < Count; i++){
        Dst[i] = Src[i];
    }
}

template<typename t> inline void CopyTypeArray(t* Dst, t* Src, int Count){
    for(int CopyIndex = 0; CopyIndex < Count; CopyIndex++){
        Dst[CopyIndex] = Src[CopyIndex];
    }
}

#endif
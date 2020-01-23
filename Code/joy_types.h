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

struct Loaded_Strings{
    char** Strings;
    int Count;
};

struct Memory_Block{
    void* Base;
    size_t Used;
    size_t Total;
};

inline void InitMemoryBlock(Memory_Block* block, void* Base, mi Size){
    block->Base = Base;
    block->Used = 0;
    block->Total = Size;
}

inline Memory_Block InitMemoryBlock(void* Base, mi Size){
    Memory_Block block;
    
    block.Base = Base;
    block.Used = 0;
    block.Total = Size;
    
    return(block);
}

inline int SafeTruncateToInt(float Value){
    int Result = (int)(Value + 0.5f);
    
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
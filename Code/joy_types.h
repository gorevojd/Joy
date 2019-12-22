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

inline int SafeTruncateToInt(float Value){
    int Result = (int)(Value + 0.5f);
    
    return(Result);
}

inline void ToggleBool(b32* Value){
    *Value = !*Value;
}

#endif
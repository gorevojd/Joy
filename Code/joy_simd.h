#ifndef JOY_SIMD_H
#define JOY_SIMD_H

#if defined(JOY_AVX)

#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

#define MM4(mm, i) (mm).m128_f32[i]
#define MMI4(mm, i) (mm).m128i_u32[i]

#define MM8(mm, i) (mm).m256_f32[i]
#define MMI8(mm, i) (mm).m256i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

struct i32_8x{
    __m256i e;
};

struct f32_8x{
    __m256 e;
};

struct v2_8x{
    union{
        struct{
            
            __m256 x;
            __m256 y;
        };
        
        __m256 e[2];
    };
};

struct v3_8x{
    union{
        struct{
            __m256 x;
            __m256 y;
            __m256 z;
        };
        
        __m256 e[3];
    };
};

struct v4_8x{
    union{
        struct{
            __m256 x;
            __m256 y;
            __m256 z;
            __m256 w;
        };
        
        __m256 e[4];
    };
    
};

struct m44_8x{
    union{
        struct{
            __m256 m00;
            __m256 m01;
            __m256 m02;
            __m256 m03;
            
            __m256 m10;
            __m256 m11;
            __m256 m12;
            __m256 m13;
            
            __m256 m20;
            __m256 m21;
            __m256 m22;
            __m256 m23;
            
            __m256 m30;
            __m256 m31;
            __m256 m32;
            __m256 m33;
        };
        
        __m256 m[16];
    };
};

// NOTE(Dima): Contstructors
inline i32_8x I32_8X(int A){
    i32_8x Result;
    
    Result.e = _mm256_set1_epi32(A);
    
    return(Result);
}

inline i32_8x I32_8X(int A, int B, int C, int D,
                     int E, int F, int G, int H)
{
    i32_8x Result;
    
    Result.e = _mm256_setr_epi32(A, B, C, D, E, F, G, H);
    
    return(Result);
}

inline i32_8x IndicesStartFrom(int StartIndex, int Step = 1)
{
    i32_8x Result;
    
    int Index0 = StartIndex + 0 * Step;
    int Index1 = StartIndex + 1 * Step;
    int Index2 = StartIndex + 2 * Step;
    int Index3 = StartIndex + 3 * Step;
    int Index4 = StartIndex + 4 * Step;
    int Index5 = StartIndex + 5 * Step;
    int Index6 = StartIndex + 6 * Step;
    int Index7 = StartIndex + 7 * Step;
    
    Result.e = _mm256_setr_epi32(Index0,
                                 Index1,
                                 Index2,
                                 Index3,
                                 Index4,
                                 Index5,
                                 Index6,
                                 Index7);
    
    return(Result);
}

inline i32_8x I32_8X(__m256i Value){
    i32_8x Result;
    
    Result.e = Value;
    
    return(Result);
}


inline f32_8x F32_8X(f32 A){
    f32_8x Result;
    
    Result.e = _mm256_set1_ps(A);
    
    return(Result);
}

inline f32_8x F32_8X(f32 A, f32 B, f32 C, f32 D,
                     f32 E, f32 F, f32 G, f32 H)
{
    f32_8x Result;
    
    Result.e = _mm256_setr_ps(A, B, C, D, E, F, G, H);
    
    return(Result);
}


inline f32_8x F32_8X(__m256 Value){
    f32_8x Result;
    
    Result.e = Value;
    
    return(Result);
}

#define JOY_SIMD_QUICK_PARAM_FILL(after_dot) _mm256_setr_ps(A.##after_dot, \
B.##after_dot, \
C.##after_dot, \
D.##after_dot, \
E.##after_dot, \
F.##after_dot, \
G.##after_dot, \
H.##after_dot)

inline v2_8x V2_8X(const v2& A,
                   const v2& B,
                   const v2& C,
                   const v2& D,
                   const v2& E,
                   const v2& F,
                   const v2& G,
                   const v2& H)
{
    v2_8x Result;
    
    Result.x = JOY_SIMD_QUICK_PARAM_FILL(x);
    Result.y = JOY_SIMD_QUICK_PARAM_FILL(y);
    
    return(Result);
}

inline v3_8x V3_8X(const v3& A,
                   const v3& B,
                   const v3& C,
                   const v3& D,
                   const v3& E,
                   const v3& F,
                   const v3& G,
                   const v3& H)
{
    v3_8x Result;
    
    Result.x = JOY_SIMD_QUICK_PARAM_FILL(x);
    Result.y = JOY_SIMD_QUICK_PARAM_FILL(y);
    Result.z = JOY_SIMD_QUICK_PARAM_FILL(z);
    
    return(Result);
}

inline v3_8x V3_8X(const v3& A)
{
    v3_8x Result;
    
    Result.x = _mm256_set1_ps(A.x);
    Result.y = _mm256_set1_ps(A.y);
    Result.z = _mm256_set1_ps(A.z);
    
    return(Result);
}

inline v4_8x V4_8X(const v4& A,
                   const v4& B,
                   const v4& C,
                   const v4& D,
                   const v4& E,
                   const v4& F,
                   const v4& G,
                   const v4& H)
{
    v4_8x Result;
    
    Result.x = JOY_SIMD_QUICK_PARAM_FILL(x);
    Result.y = JOY_SIMD_QUICK_PARAM_FILL(y);
    Result.z = JOY_SIMD_QUICK_PARAM_FILL(z);
    Result.w = JOY_SIMD_QUICK_PARAM_FILL(w);
    
    return(Result);
}

inline v4_8x V4_8X(const v4& A)
{
    v4_8x Result;
    
    Result.x = _mm256_set1_ps(A.x);
    Result.y = _mm256_set1_ps(A.y);
    Result.z = _mm256_set1_ps(A.z);
    Result.w = _mm256_set1_ps(A.w);
    
    return(Result);
}

inline v4_8x V4_8X(const quat& A)
{
    v4_8x Result;
    
    Result.x = _mm256_set1_ps(A.x);
    Result.y = _mm256_set1_ps(A.y);
    Result.z = _mm256_set1_ps(A.z);
    Result.w = _mm256_set1_ps(A.w);
    
    return(Result);
}

inline v4_8x V4_8X(const quat& A,
                   const quat& B,
                   const quat& C,
                   const quat& D,
                   const quat& E,
                   const quat& F,
                   const quat& G,
                   const quat& H)
{
    v4_8x Result;
    
    Result.x = JOY_SIMD_QUICK_PARAM_FILL(x);
    Result.y = JOY_SIMD_QUICK_PARAM_FILL(y);
    Result.z = JOY_SIMD_QUICK_PARAM_FILL(z);
    Result.w = JOY_SIMD_QUICK_PARAM_FILL(w);
    
    return(Result);
}

inline m44_8x M44_8X(const m44& A,
                     const m44& B,
                     const m44& C,
                     const m44& D,
                     const m44& E,
                     const m44& F,
                     const m44& G,
                     const m44& H)
{
    m44_8x Result;
    
    Result.m[0] = JOY_SIMD_QUICK_PARAM_FILL(e[0]);
    Result.m[1] = JOY_SIMD_QUICK_PARAM_FILL(e[1]);
    Result.m[2] = JOY_SIMD_QUICK_PARAM_FILL(e[2]);
    Result.m[3] = JOY_SIMD_QUICK_PARAM_FILL(e[3]);
    
    Result.m[4] = JOY_SIMD_QUICK_PARAM_FILL(e[4]);
    Result.m[5] = JOY_SIMD_QUICK_PARAM_FILL(e[5]);
    Result.m[6] = JOY_SIMD_QUICK_PARAM_FILL(e[6]);
    Result.m[7] = JOY_SIMD_QUICK_PARAM_FILL(e[7]);
    
    Result.m[8] = JOY_SIMD_QUICK_PARAM_FILL(e[8]);
    Result.m[9] = JOY_SIMD_QUICK_PARAM_FILL(e[9]);
    Result.m[10] = JOY_SIMD_QUICK_PARAM_FILL(e[10]);
    Result.m[11] = JOY_SIMD_QUICK_PARAM_FILL(e[11]);
    
    Result.m[12] = JOY_SIMD_QUICK_PARAM_FILL(e[12]);
    Result.m[13] = JOY_SIMD_QUICK_PARAM_FILL(e[13]);
    Result.m[14] = JOY_SIMD_QUICK_PARAM_FILL(e[14]);
    Result.m[15] = JOY_SIMD_QUICK_PARAM_FILL(e[15]);
    
    return(Result);
}

inline m44_8x TranslationMatrix(const v3& A,
                                const v3& B,
                                const v3& C,
                                const v3& D,
                                const v3& E,
                                const v3& F,
                                const v3& G,
                                const v3& H)
{
    m44_8x Result;
    
    __m256 Zero = _mm256_set1_ps(0.0f);
    __m256 One = _mm256_set1_ps(1.0f);
    
    Result.m[0] = One;
    Result.m[1] = Zero;
    Result.m[2] = Zero;
    Result.m[3] = Zero;
    
    Result.m[4] = Zero;
    Result.m[5] = One;
    Result.m[6] = Zero;
    Result.m[7] = Zero;
    
    Result.m[8] = Zero;
    Result.m[9] = Zero;
    Result.m[10] = One;
    Result.m[11] = Zero;
    
    Result.m[12] = JOY_SIMD_QUICK_PARAM_FILL(x);
    Result.m[13] = JOY_SIMD_QUICK_PARAM_FILL(y);
    Result.m[14] = JOY_SIMD_QUICK_PARAM_FILL(z);
    Result.m[15] = One;
    
    return(Result);
}

inline m44_8x ScalingMatrix(const v3& A,
                            const v3& B,
                            const v3& C,
                            const v3& D,
                            const v3& E,
                            const v3& F,
                            const v3& G,
                            const v3& H)
{
    m44_8x Result;
    
    __m256 Zero = _mm256_set1_ps(0.0f);
    
    __m256 x = JOY_SIMD_QUICK_PARAM_FILL(x);
    __m256 y = JOY_SIMD_QUICK_PARAM_FILL(y);
    __m256 z = JOY_SIMD_QUICK_PARAM_FILL(z);
    
    Result.m[0] = x;
    Result.m[1] = Zero;
    Result.m[2] = Zero;
    Result.m[3] = Zero;
    
    Result.m[4] = Zero;
    Result.m[5] = y;
    Result.m[6] = Zero;
    Result.m[7] = Zero;
    
    Result.m[8] = Zero;
    Result.m[9] = Zero;
    Result.m[10] = z;
    Result.m[11] = Zero;
    
    Result.m[12] = Zero;
    Result.m[13] = Zero;
    Result.m[14] = Zero;
    Result.m[15] = _mm256_set1_ps(1.0f);
    
    return(Result);
}

inline m44_8x RotationMatrix(const quat& A,
                             const quat& B,
                             const quat& C,
                             const quat& D,
                             const quat& E,
                             const quat& F,
                             const quat& G,
                             const quat& H)
{
    m44_8x Result;
    
    __m256 Zero = _mm256_set1_ps(0.0f);
    __m256 One = _mm256_set1_ps(1.0f);
    __m256 Two = _mm256_set1_ps(2.0f);
    
    __m256 x = JOY_SIMD_QUICK_PARAM_FILL(x);
    __m256 y = JOY_SIMD_QUICK_PARAM_FILL(y);
    __m256 z = JOY_SIMD_QUICK_PARAM_FILL(z);
    __m256 w = JOY_SIMD_QUICK_PARAM_FILL(w);
    
    __m256 xx = _mm256_mul_ps(x, x);
    __m256 yy = _mm256_mul_ps(y, y);
    __m256 zz = _mm256_mul_ps(z, z);
    
    __m256 xy = _mm256_mul_ps(x, y);
    __m256 zw = _mm256_mul_ps(z, w);
    __m256 xz = _mm256_mul_ps(x, z);
    __m256 yw = _mm256_mul_ps(y, w);
    __m256 yz = _mm256_mul_ps(y, z);
    __m256 xw = _mm256_mul_ps(x, w);
    
    Result.m[0] = _mm256_sub_ps(One, _mm256_mul_ps(Two, _mm256_add_ps(yy, zz)));
    Result.m[1] = _mm256_mul_ps(Two, _mm256_add_ps(xy, zw));
    Result.m[2] = _mm256_mul_ps(Two, _mm256_sub_ps(xz, yw));
    Result.m[3] = Zero;
    
    Result.m[4] = _mm256_mul_ps(Two, _mm256_sub_ps(xy, zw));
    Result.m[5] = _mm256_sub_ps(One, _mm256_mul_ps(Two, _mm256_add_ps(xx, zz)));
    Result.m[6] = _mm256_mul_ps(Two, _mm256_add_ps(yz, xw));
    Result.m[7] = Zero;
    
    Result.m[8] = _mm256_mul_ps(Two, _mm256_add_ps(xz, yw));
    Result.m[9] = _mm256_mul_ps(Two, _mm256_sub_ps(yz, xw));
    Result.m[10] = _mm256_sub_ps(One, _mm256_mul_ps(Two, _mm256_add_ps(xx, yy)));
    Result.m[11] = Zero;
    
    Result.m[12] = Zero;
    Result.m[13] = Zero;
    Result.m[14] = Zero;
    Result.m[15] = One;
    
    return(Result);
}


// NOTE(Dima): Store
inline void I32_8X_Store(i32_8x ToStore,
                         int* A, int* B,
                         int* C, int* D,
                         int* E, int* F,
                         int* G, int* H)
{
    *A = MMI8(ToStore.e, 0);
    *B = MMI8(ToStore.e, 1);
    *C = MMI8(ToStore.e, 2);
    *D = MMI8(ToStore.e, 3);
    *E = MMI8(ToStore.e, 4);
    *F = MMI8(ToStore.e, 5);
    *G = MMI8(ToStore.e, 6);
    *H = MMI8(ToStore.e, 7);
}

inline void F32_8X_Store(f32_8x ToStore,
                         f32* A, f32* B,
                         f32* C, f32* D,
                         f32* E, f32* F,
                         f32* G, f32* H)
{
    *A = MM8(ToStore.e, 0);
    *B = MM8(ToStore.e, 1);
    *C = MM8(ToStore.e, 2);
    *D = MM8(ToStore.e, 3);
    *E = MM8(ToStore.e, 4);
    *F = MM8(ToStore.e, 5);
    *G = MM8(ToStore.e, 6);
    *H = MM8(ToStore.e, 7);
}

inline void V3_8X_Store(const v3_8x& Ref, 
                        v3* A,
                        v3* B,
                        v3* C,
                        v3* D,
                        v3* E,
                        v3* F,
                        v3* G,
                        v3* H)
{
    A->x = MM8(Ref.x, 0);
    A->y = MM8(Ref.y, 0);
    A->z = MM8(Ref.z, 0);
    
    B->x = MM8(Ref.x, 1);
    B->y = MM8(Ref.y, 1);
    B->z = MM8(Ref.z, 1);
    
    C->x = MM8(Ref.x, 2);
    C->y = MM8(Ref.y, 2);
    C->z = MM8(Ref.z, 2);
    
    D->x = MM8(Ref.x, 3);
    D->y = MM8(Ref.y, 3);
    D->z = MM8(Ref.z, 3);
    
    E->x = MM8(Ref.x, 4);
    E->y = MM8(Ref.y, 4);
    E->z = MM8(Ref.z, 4);
    
    F->x = MM8(Ref.x, 5);
    F->y = MM8(Ref.y, 5);
    F->z = MM8(Ref.z, 5);
    
    G->x = MM8(Ref.x, 6);
    G->y = MM8(Ref.y, 6);
    G->z = MM8(Ref.z, 6);
    
    H->x = MM8(Ref.x, 7);
    H->y = MM8(Ref.y, 7);
    H->z = MM8(Ref.z, 7);
}

inline void V4_8X_Store(const v4_8x& Ref, 
                        v4* A,
                        v4* B,
                        v4* C,
                        v4* D,
                        v4* E,
                        v4* F,
                        v4* G,
                        v4* H)
{
    A->x = MM8(Ref.x, 0);
    A->y = MM8(Ref.y, 0);
    A->z = MM8(Ref.z, 0);
    A->w = MM8(Ref.w, 0);
    
    B->x = MM8(Ref.x, 1);
    B->y = MM8(Ref.y, 1);
    B->z = MM8(Ref.z, 1);
    B->w = MM8(Ref.w, 1);
    
    C->x = MM8(Ref.x, 2);
    C->y = MM8(Ref.y, 2);
    C->z = MM8(Ref.z, 2);
    C->w = MM8(Ref.w, 2);
    
    D->x = MM8(Ref.x, 3);
    D->y = MM8(Ref.y, 3);
    D->z = MM8(Ref.z, 3);
    D->w = MM8(Ref.w, 3);
    
    E->x = MM8(Ref.x, 4);
    E->y = MM8(Ref.y, 4);
    E->z = MM8(Ref.z, 4);
    E->w = MM8(Ref.w, 4);
    
    F->x = MM8(Ref.x, 5);
    F->y = MM8(Ref.y, 5);
    F->z = MM8(Ref.z, 5);
    F->w = MM8(Ref.w, 5);
    
    G->x = MM8(Ref.x, 6);
    G->y = MM8(Ref.y, 6);
    G->z = MM8(Ref.z, 6);
    G->w = MM8(Ref.w, 6);
    
    H->x = MM8(Ref.x, 7);
    H->y = MM8(Ref.y, 7);
    H->z = MM8(Ref.z, 7);
    H->w = MM8(Ref.w, 7);
}

inline void V4_8X_Store(const v4_8x& Ref, 
                        quat* A,
                        quat* B,
                        quat* C,
                        quat* D,
                        quat* E,
                        quat* F,
                        quat* G,
                        quat* H)
{
    A->x = MM8(Ref.x, 0);
    A->y = MM8(Ref.y, 0);
    A->z = MM8(Ref.z, 0);
    A->w = MM8(Ref.w, 0);
    
    B->x = MM8(Ref.x, 1);
    B->y = MM8(Ref.y, 1);
    B->z = MM8(Ref.z, 1);
    B->w = MM8(Ref.w, 1);
    
    C->x = MM8(Ref.x, 2);
    C->y = MM8(Ref.y, 2);
    C->z = MM8(Ref.z, 2);
    C->w = MM8(Ref.w, 2);
    
    D->x = MM8(Ref.x, 3);
    D->y = MM8(Ref.y, 3);
    D->z = MM8(Ref.z, 3);
    D->w = MM8(Ref.w, 3);
    
    E->x = MM8(Ref.x, 4);
    E->y = MM8(Ref.y, 4);
    E->z = MM8(Ref.z, 4);
    E->w = MM8(Ref.w, 4);
    
    F->x = MM8(Ref.x, 5);
    F->y = MM8(Ref.y, 5);
    F->z = MM8(Ref.z, 5);
    F->w = MM8(Ref.w, 5);
    
    G->x = MM8(Ref.x, 6);
    G->y = MM8(Ref.y, 6);
    G->z = MM8(Ref.z, 6);
    G->w = MM8(Ref.w, 6);
    
    H->x = MM8(Ref.x, 7);
    H->y = MM8(Ref.y, 7);
    H->z = MM8(Ref.z, 7);
    H->w = MM8(Ref.w, 7);
}

#define M44_STORE_INDEX(i)\
A->e[i] = MM8(Ref.m[i], 0);\
B->e[i] = MM8(Ref.m[i], 1);\
C->e[i] = MM8(Ref.m[i], 2);\
D->e[i] = MM8(Ref.m[i], 3);\
E->e[i] = MM8(Ref.m[i], 4);\
F->e[i] = MM8(Ref.m[i], 5);\
G->e[i] = MM8(Ref.m[i], 6);\
H->e[i] = MM8(Ref.m[i], 7);

inline void M44_8X_Store(const m44_8x& Ref,
                         m44* A, m44* B,
                         m44* C, m44* D,
                         m44* E, m44* F,
                         m44* G, m44* H)
{
    M44_STORE_INDEX(0);
    M44_STORE_INDEX(1);
    M44_STORE_INDEX(2);
    M44_STORE_INDEX(3);
    M44_STORE_INDEX(4);
    M44_STORE_INDEX(5);
    M44_STORE_INDEX(6);
    M44_STORE_INDEX(7);
    M44_STORE_INDEX(8);
    M44_STORE_INDEX(9);
    M44_STORE_INDEX(10);
    M44_STORE_INDEX(11);
    M44_STORE_INDEX(12);
    M44_STORE_INDEX(13);
    M44_STORE_INDEX(14);
    M44_STORE_INDEX(15);
}

inline void I32_8X_StoreConditional(i32_8x ToStore,
                                    int MovedMask,
                                    int* StoreTo[8])
{
    for(int i = 0; i < 8; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            *StoreTo[i] = MMI8(ToStore.e, i);
        }
    }
}

inline void F32_8X_StoreConditional(f32_8x ToStore,
                                    int MovedMask,
                                    f32* StoreTo[8])
{
    for(int i = 0; i < 8; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            *StoreTo[i] = MM8(ToStore.e, i);
        }
    }
}

inline void V3_8X_StoreConditional(const v3_8x& Ref,
                                   int MovedMask,
                                   v3* StoreTo[8])
{
    for(int i = 0; i < 8; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM8(Ref.e[0], i);
            StoreTo[i]->y = MM8(Ref.e[1], i);
            StoreTo[i]->z = MM8(Ref.e[2], i);
        }
    }
}

inline void V4_8X_StoreConditional(const v4_8x& Ref,
                                   int MovedMask,
                                   v4* StoreTo[8])
{
    for(int i = 0; i < 8; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM8(Ref.e[0], i);
            StoreTo[i]->y = MM8(Ref.e[1], i);
            StoreTo[i]->z = MM8(Ref.e[2], i);
            StoreTo[i]->w = MM8(Ref.e[3], i);
        }
    }
}

inline void V4_8X_StoreConditional(const v4_8x& Ref,
                                   int MovedMask,
                                   quat* StoreTo[8])
{
    for(int i = 0; i < 8; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM8(Ref.e[0], i);
            StoreTo[i]->y = MM8(Ref.e[1], i);
            StoreTo[i]->z = MM8(Ref.e[2], i);
            StoreTo[i]->w = MM8(Ref.e[3], i);
        }
    }
}

// NOTE(Dima): Conversions
inline f32_8x CastToFloat(i32_8x ToConvert){
    f32_8x Result;
    
    Result.e = _mm256_castsi256_ps(ToConvert.e);
    
    return(Result);
}

inline i32_8x CastToInt(f32_8x ToConvert){
    i32_8x Result;
    
    Result.e = _mm256_castps_si256(ToConvert.e);
    
    return(Result);
}

inline f32_8x ConvertToFloat(i32_8x ToConvert){
    f32_8x Result;
    
    Result.e = _mm256_cvtepi32_ps(ToConvert.e);
    
    return(Result);
}

inline i32_8x ConvertToInt(f32_8x ToConvert){
    i32_8x Result;
    
    Result.e = _mm256_cvtps_epi32(ToConvert.e);
    
    return(Result);
}

// NOTE(Dima): Boolean operations

inline f32_8x IsTrue(const i32_8x& A){
    __m256 Zero = _mm256_set1_ps(0.0f);
    
    f32_8x Result;
    Result.e = _mm256_cmp_ps(_mm256_cvtepi32_ps(A.e), Zero, _CMP_NEQ_OQ);
    
    return(Result);
}

inline f32_8x IsEqual(const i32_8x& A, const i32_8x& EqTo){
    f32_8x Result;
    
    Result.e = _mm256_castsi256_ps(_mm256_cmpeq_epi32(A.e, EqTo.e));
    
    return(Result);
}

inline f32_8x LessThan(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_cmp_ps(A.e, B.e, _CMP_LT_OQ);
    
    return(Result);
}


inline f32_8x GreaterThan(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_cmp_ps(A.e, B.e, _CMP_GT_OQ);
    
    return(Result);
}

inline f32_8x GreaterEqThan(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_cmp_ps(A.e, B.e, _CMP_GE_OQ);
    
    return(Result);
}

inline f32_8x SignNotZero(const f32_8x& A){
    __m256 Mask = _mm256_cmp_ps(A.e, _mm256_set1_ps(0.0f), _CMP_GT_OQ);
    
    f32_8x Result;
    Result.e = _mm256_or_ps(_mm256_and_ps(Mask, _mm256_set1_ps(1.0f)),
                            _mm256_andnot_ps(Mask, _mm256_set1_ps(-1.0f)));
    
    return(Result);
}

inline int AnyTrue(const f32_8x& A){
    int Result = _mm256_movemask_ps(A.e);
    
    return(Result);
}

inline int GetMask(const f32_8x& A){
    int Result = _mm256_movemask_ps(A.e);
    
    return(Result);
}

// NOTE(Dima): Addition
inline f32_8x operator+(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    Result.e = _mm256_add_ps(A.e, B.e);
    return(Result);
}

inline v2_8x operator+(const v2_8x& A, const v2_8x& B){
    v2_8x Result;
    
    Result.x = _mm256_add_ps(A.x, B.x);
    Result.y = _mm256_add_ps(A.y, B.y);
    
    return(Result);
}


inline v3_8x operator+(const v3_8x& A, const v3_8x& B){
    v3_8x Result;
    
    Result.x = _mm256_add_ps(A.x, B.x);
    Result.y = _mm256_add_ps(A.y, B.y);
    Result.z = _mm256_add_ps(A.z, B.z);
    
    return(Result);
}


inline v4_8x operator+(const v4_8x& A, const v4_8x& B){
    v4_8x Result;
    
    Result.x = _mm256_add_ps(A.x, B.x);
    Result.y = _mm256_add_ps(A.y, B.y);
    Result.z = _mm256_add_ps(A.z, B.z);
    Result.w = _mm256_add_ps(A.w, B.w);
    
    return(Result);
}

// NOTE(Dima): Add equal
inline f32_8x& operator+=(f32_8x& A, const f32_8x& B){
    A.e = _mm256_add_ps(A.e, B.e);
    return(A);
}

inline v2_8x& operator+=(v2_8x& A, const v2_8x& B){
    A.x = _mm256_add_ps(A.x, B.x);
    A.y = _mm256_add_ps(A.y, B.y);
    
    return(A);
}

inline v3_8x& operator+=(v3_8x& A, const v3_8x& B){
    A.x = _mm256_add_ps(A.x, B.x);
    A.y = _mm256_add_ps(A.y, B.y);
    A.z = _mm256_add_ps(A.z, B.z);
    
    return(A);
}

inline v4_8x& operator+=(v4_8x& A, const v4_8x& B){
    A.x = _mm256_add_ps(A.x, B.x);
    A.y = _mm256_add_ps(A.y, B.y);
    A.z = _mm256_add_ps(A.z, B.z);
    A.w = _mm256_add_ps(A.w, B.w);
    
    return(A);
}

// NOTE(Dima): Subtraction
inline f32_8x operator-(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    Result.e = _mm256_sub_ps(A.e, B.e);
    return(Result);
}

inline v2_8x operator-(const v2_8x& A, const v2_8x& B){
    v2_8x Result;
    
    Result.x = _mm256_sub_ps(A.x, B.x);
    Result.y = _mm256_sub_ps(A.y, B.y);
    
    return(Result);
}


inline v3_8x operator-(const v3_8x& A, const v3_8x& B){
    v3_8x Result;
    
    Result.x = _mm256_sub_ps(A.x, B.x);
    Result.y = _mm256_sub_ps(A.y, B.y);
    Result.z = _mm256_sub_ps(A.z, B.z);
    
    return(Result);
}


inline v4_8x operator-(const v4_8x& A, const v4_8x& B){
    v4_8x Result;
    
    Result.x = _mm256_sub_ps(A.x, B.x);
    Result.y = _mm256_sub_ps(A.y, B.y);
    Result.z = _mm256_sub_ps(A.z, B.z);
    Result.w = _mm256_sub_ps(A.w, B.w);
    
    return(Result);
}

// NOTE(Dima): Multiplication
inline f32_8x operator*(const f32_8x& A, const f32_8x& B){
    f32_8x Result;
    Result.e = _mm256_mul_ps(A.e, B.e);
    return(Result);
}

inline v2_8x operator*(const v2_8x& A, const v2_8x& B){
    v2_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.x);
    Result.y = _mm256_mul_ps(A.y, B.y);
    
    return(Result);
}


inline v3_8x operator*(const v3_8x& A, const v3_8x& B){
    v3_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.x);
    Result.y = _mm256_mul_ps(A.y, B.y);
    Result.z = _mm256_mul_ps(A.z, B.z);
    
    return(Result);
}


inline v4_8x operator*(const v4_8x& A, const v4_8x& B){
    v4_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.x);
    Result.y = _mm256_mul_ps(A.y, B.y);
    Result.z = _mm256_mul_ps(A.z, B.z);
    Result.w = _mm256_mul_ps(A.w, B.w);
    
    return(Result);
}

inline m44_8x operator*(const m44_8x& A, const m44_8x& B){
    m44_8x Result;
    
    // NOTE(Dima): First row
    Result.m[0] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[0], B.m[0]),
                                              _mm256_mul_ps(A.m[1], B.m[4])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[2], B.m[8]),
                                              _mm256_mul_ps(A.m[3], B.m[12])));
    
    Result.m[1] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[0], B.m[1]),
                                              _mm256_mul_ps(A.m[1], B.m[5])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[2], B.m[9]),
                                              _mm256_mul_ps(A.m[3], B.m[13])));
    
    Result.m[2] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[0], B.m[2]),
                                              _mm256_mul_ps(A.m[1], B.m[6])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[2], B.m[10]),
                                              _mm256_mul_ps(A.m[3], B.m[14])));
    
    Result.m[3] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[0], B.m[3]),
                                              _mm256_mul_ps(A.m[1], B.m[7])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[2], B.m[11]),
                                              _mm256_mul_ps(A.m[3], B.m[15])));
    
    // NOTE(Dima): Second row
    Result.m[4] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[4], B.m[0]),
                                              _mm256_mul_ps(A.m[5], B.m[4])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[6], B.m[8]),
                                              _mm256_mul_ps(A.m[7], B.m[12])));
    
    Result.m[5] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[4], B.m[1]),
                                              _mm256_mul_ps(A.m[5], B.m[5])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[6], B.m[9]),
                                              _mm256_mul_ps(A.m[7], B.m[13])));
    
    Result.m[6] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[4], B.m[2]),
                                              _mm256_mul_ps(A.m[5], B.m[6])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[6], B.m[10]),
                                              _mm256_mul_ps(A.m[7], B.m[14])));
    
    Result.m[7] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[4], B.m[3]),
                                              _mm256_mul_ps(A.m[5], B.m[7])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[6], B.m[11]),
                                              _mm256_mul_ps(A.m[7], B.m[15])));
    
    
    // NOTE(Dima): Third row
    Result.m[8] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[8], B.m[0]),
                                              _mm256_mul_ps(A.m[9], B.m[4])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[10], B.m[8]),
                                              _mm256_mul_ps(A.m[11], B.m[12])));
    
    Result.m[9] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[8], B.m[1]),
                                              _mm256_mul_ps(A.m[9], B.m[5])),
                                _mm256_add_ps(_mm256_mul_ps(A.m[10], B.m[9]),
                                              _mm256_mul_ps(A.m[11], B.m[13])));
    
    Result.m[10] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[8], B.m[2]),
                                               _mm256_mul_ps(A.m[9], B.m[6])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[10], B.m[10]),
                                               _mm256_mul_ps(A.m[11], B.m[14])));
    
    Result.m[11] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[8], B.m[3]),
                                               _mm256_mul_ps(A.m[9], B.m[7])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[10], B.m[11]),
                                               _mm256_mul_ps(A.m[11], B.m[15])));
    
    
    // NOTE(Dima): Fourth row
    Result.m[12] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[12], B.m[0]),
                                               _mm256_mul_ps(A.m[13], B.m[4])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[14], B.m[8]),
                                               _mm256_mul_ps(A.m[15], B.m[12])));
    
    Result.m[13] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[12], B.m[1]),
                                               _mm256_mul_ps(A.m[13], B.m[5])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[14], B.m[9]),
                                               _mm256_mul_ps(A.m[15], B.m[13])));
    
    Result.m[14] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[12], B.m[2]),
                                               _mm256_mul_ps(A.m[13], B.m[6])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[14], B.m[10]),
                                               _mm256_mul_ps(A.m[15], B.m[14])));
    
    Result.m[15] = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.m[12], B.m[3]),
                                               _mm256_mul_ps(A.m[13], B.m[7])),
                                 _mm256_add_ps(_mm256_mul_ps(A.m[14], B.m[11]),
                                               _mm256_mul_ps(A.m[15], B.m[15])));
    
    return(Result);
}

// NOTE(Dima): Multiplication by a scalar
inline v2_8x operator*(const v2_8x& A, const f32_8x& B){
    v2_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.e);
    Result.y = _mm256_mul_ps(A.y, B.e);
    
    return(Result);
}


inline v3_8x operator*(const v3_8x& A, const f32_8x& B){
    v3_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.e);
    Result.y = _mm256_mul_ps(A.y, B.e);
    Result.z = _mm256_mul_ps(A.z, B.e);
    
    return(Result);
}


inline v4_8x operator*(const v4_8x& A, const f32_8x& B){
    v4_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, B.e);
    Result.y = _mm256_mul_ps(A.y, B.e);
    Result.z = _mm256_mul_ps(A.z, B.e);
    Result.w = _mm256_mul_ps(A.w, B.e);
    
    return(Result);
}

// NOTE(Dima): Dot products
inline f32_8x Dot(const v2_8x& A, const v2_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_add_ps(_mm256_mul_ps(A.x, B.x),
                             _mm256_mul_ps(A.y, B.y));
    
    return(Result);
}

inline f32_8x Dot(const v3_8x& A, const v3_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.x, B.x),
                                           _mm256_mul_ps(A.y, B.y)),
                             _mm256_mul_ps(A.z, B.z));
    
    return(Result);
}

inline f32_8x Dot(const v4_8x& A, const v4_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.x, B.x),
                                           _mm256_mul_ps(A.y, B.y)),
                             _mm256_add_ps(_mm256_mul_ps(A.z, B.z),
                                           _mm256_mul_ps(A.w, B.w)));
    
    return(Result);
}



// NOTE(Dima): Normalize ops
inline v2_8x Normalize(const v2_8x& A){
    __m256 SqLen = _mm256_add_ps(_mm256_mul_ps(A.x, A.x),
                                 _mm256_mul_ps(A.y, A.y));
    
    __m256 OneOverLen = _mm256_rsqrt_ps(SqLen);
    
    v2_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, OneOverLen);
    Result.y = _mm256_mul_ps(A.y, OneOverLen);
    
    return(Result);
}

inline v3_8x Normalize(const v3_8x& A){
    __m256 SqLen = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.x, A.x),
                                               _mm256_mul_ps(A.y, A.y)),
                                 _mm256_mul_ps(A.z, A.z));
    
    __m256 OneOverLen = _mm256_rsqrt_ps(SqLen);
    
    v3_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, OneOverLen);
    Result.y = _mm256_mul_ps(A.y, OneOverLen);
    Result.z = _mm256_mul_ps(A.z, OneOverLen);
    
    return(Result);
}

inline v4_8x Normalize(const v4_8x& A){
    __m256 SqLen = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(A.x, A.x),
                                               _mm256_mul_ps(A.y, A.y)),
                                 _mm256_add_ps(_mm256_mul_ps(A.z, A.z),
                                               _mm256_mul_ps(A.w, A.w)));
    
    __m256 OneOverLen = _mm256_rsqrt_ps(SqLen);
    
    v4_8x Result;
    
    Result.x = _mm256_mul_ps(A.x, OneOverLen);
    Result.y = _mm256_mul_ps(A.y, OneOverLen);
    Result.z = _mm256_mul_ps(A.z, OneOverLen);
    Result.w = _mm256_mul_ps(A.w, OneOverLen);
    
    return(Result);
}

// NOTE(Dima): 
inline __m256 ConditionalCombineInternal(__m256 Mask, __m256 A, __m256 B)
{
    __m256 Result = _mm256_or_ps(_mm256_and_ps(Mask, A),
                                 _mm256_andnot_ps(Mask, B));
    
    return(Result);
}

// NOTE(Dima): 1 - A, 0 - B
inline f32_8x Blend(const f32_8x& Mask, const f32_8x&  A, const f32_8x& B){
    f32_8x Result;
    
    Result.e = _mm256_or_ps(_mm256_and_ps(Mask.e, A.e),
                            _mm256_andnot_ps(Mask.e, B.e));
    
    return(Result);
}

// NOTE(Dima): 1 - A, 0 - B
inline i32_8x Blend(const f32_8x& Mask, const i32_8x& A, const i32_8x& B){
    i32_8x Result;
    
    __m256i MaskI = _mm256_castps_si256(Mask.e);
    
    Result.e = _mm256_or_si256(_mm256_and_si256(MaskI, A.e),
                               _mm256_andnot_si256(MaskI, B.e));
    
    return(Result);
}

inline f32_8x ConditionalCombine(f32_8x Mask,
                                 f32_8x A,
                                 f32_8x B)
{
    f32_8x Result;
    
    Result.e = _mm256_or_ps(_mm256_and_ps(Mask.e, A.e),
                            _mm256_andnot_ps(Mask.e, B.e));
    
    return(Result);
}

inline v2_8x ConditionalCombine(f32_8x Mask, 
                                const v2_8x& A, 
                                const v2_8x& B)
{
    v2_8x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    
    return(Result);
}

inline v3_8x ConditionalCombine(f32_8x Mask, 
                                const v3_8x& A, 
                                const v3_8x& B)
{
    v3_8x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    Result.z = ConditionalCombineInternal(Mask.e, A.z, B.z);
    
    return(Result);
}

inline v4_8x ConditionalCombine(f32_8x Mask, 
                                const v4_8x& A, 
                                const v4_8x& B)
{
    v4_8x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    Result.z = ConditionalCombineInternal(Mask.e, A.z, B.z);
    Result.w = ConditionalCombineInternal(Mask.e, A.w, B.w);
    
    return(Result);
}

inline m44_8x ConditionalCombine(f32_8x Mask,
                                 const m44_8x& A,
                                 const m44_8x& B)
{
    m44_8x Result;
    
    Result.m[0] = ConditionalCombineInternal(Mask.e, A.m[0], B.m[0]);
    Result.m[1] = ConditionalCombineInternal(Mask.e, A.m[1], B.m[1]);
    Result.m[2] = ConditionalCombineInternal(Mask.e, A.m[2], B.m[2]);
    Result.m[3] = ConditionalCombineInternal(Mask.e, A.m[3], B.m[3]);
    Result.m[4] = ConditionalCombineInternal(Mask.e, A.m[4], B.m[4]);
    Result.m[5] = ConditionalCombineInternal(Mask.e, A.m[5], B.m[5]);
    Result.m[6] = ConditionalCombineInternal(Mask.e, A.m[6], B.m[6]);
    Result.m[7] = ConditionalCombineInternal(Mask.e, A.m[7], B.m[7]);
    Result.m[8] = ConditionalCombineInternal(Mask.e, A.m[8], B.m[8]);
    Result.m[9] = ConditionalCombineInternal(Mask.e, A.m[9], B.m[9]);
    Result.m[10] = ConditionalCombineInternal(Mask.e, A.m[10], B.m[10]);
    Result.m[11] = ConditionalCombineInternal(Mask.e, A.m[11], B.m[11]);
    Result.m[12] = ConditionalCombineInternal(Mask.e, A.m[12], B.m[12]);
    Result.m[13] = ConditionalCombineInternal(Mask.e, A.m[13], B.m[13]);
    Result.m[14] = ConditionalCombineInternal(Mask.e, A.m[14], B.m[14]);
    Result.m[15] = ConditionalCombineInternal(Mask.e, A.m[15], B.m[15]);
    
    return(Result);
}


// NOTE(Dima): Lerp ops
inline v2_8x Lerp(const v2_8x& A,
                  const v2_8x& B,
                  const f32_8x& t)
{
    v2_8x Result;
    
    Result.x = _mm256_add_ps(A.x, _mm256_mul_ps(_mm256_sub_ps(B.x, A.x), t.e));
    Result.y = _mm256_add_ps(A.y, _mm256_mul_ps(_mm256_sub_ps(B.y, A.y), t.e));
    
    return(Result);
}

inline v3_8x Lerp(const v3_8x& A,
                  const v3_8x& B,
                  const f32_8x& t)
{
    v3_8x Result;
    
    Result.x = _mm256_add_ps(A.x, _mm256_mul_ps(_mm256_sub_ps(B.x, A.x), t.e));
    Result.y = _mm256_add_ps(A.y, _mm256_mul_ps(_mm256_sub_ps(B.y, A.y), t.e));
    Result.z = _mm256_add_ps(A.z, _mm256_mul_ps(_mm256_sub_ps(B.z, A.z), t.e));
    
    return(Result);
}

inline v4_8x Lerp(const v4_8x& A,
                  const v4_8x& B,
                  const f32_8x& t)
{
    v4_8x Result;
    
    Result.x = _mm256_add_ps(A.x, _mm256_mul_ps(_mm256_sub_ps(B.x, A.x), t.e));
    Result.y = _mm256_add_ps(A.y, _mm256_mul_ps(_mm256_sub_ps(B.y, A.y), t.e));
    Result.z = _mm256_add_ps(A.z, _mm256_mul_ps(_mm256_sub_ps(B.z, A.z), t.e));
    Result.w = _mm256_add_ps(A.w, _mm256_mul_ps(_mm256_sub_ps(B.w, A.w), t.e));
    
    return(Result);
}

inline v4_8x LerpQuat(const v4_8x& A,
                      const v4_8x& B,
                      const f32_8x& t)
{
    v4_8x Result;
    
    __m256 Zero = _mm256_set1_ps(0.0f);
    __m256 T = t.e;
    __m256 OneMinusT = _mm256_sub_ps(_mm256_set1_ps(1.0f), t.e);
    
    f32_8x DotRes = Dot(A, B);
    __m256 ResMask = _mm256_cmp_ps(DotRes.e, Zero, _CMP_LT_OQ);
    
    // NOTE(Dima): Building up needed B
    v4_8x NegatedB;
    NegatedB.x = _mm256_sub_ps(Zero, B.x);
    NegatedB.y = _mm256_sub_ps(Zero, B.y);
    NegatedB.z = _mm256_sub_ps(Zero, B.z);
    NegatedB.w = _mm256_sub_ps(Zero, B.w);
    
    v4_8x NewB;
    NewB.x = ConditionalCombineInternal(ResMask, NegatedB.x, B.x);
    NewB.y = ConditionalCombineInternal(ResMask, NegatedB.y, B.y);
    NewB.z = ConditionalCombineInternal(ResMask, NegatedB.z, B.z);
    NewB.w = ConditionalCombineInternal(ResMask, NegatedB.w, B.w);
    
    // NOTE(Dima): Lerping
    Result.x = _mm256_add_ps(_mm256_mul_ps(A.x, OneMinusT), _mm256_mul_ps(NewB.x, T));
    Result.y = _mm256_add_ps(_mm256_mul_ps(A.y, OneMinusT), _mm256_mul_ps(NewB.y, T));
    Result.z = _mm256_add_ps(_mm256_mul_ps(A.z, OneMinusT), _mm256_mul_ps(NewB.z, T));
    Result.w = _mm256_add_ps(_mm256_mul_ps(A.w, OneMinusT), _mm256_mul_ps(NewB.w, T));
    
    // NOTE(Dima): Normalizing
    __m256 SqLen = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(Result.x, Result.x),
                                               _mm256_mul_ps(Result.y, Result.y)),
                                 _mm256_add_ps(_mm256_mul_ps(Result.z, Result.z),
                                               _mm256_mul_ps(Result.w, Result.w)));
    
    __m256 OneOverLen = _mm256_rsqrt_ps(SqLen);
    
    Result.x = _mm256_mul_ps(Result.x, OneOverLen);
    Result.y = _mm256_mul_ps(Result.y, OneOverLen);
    Result.z = _mm256_mul_ps(Result.z, OneOverLen);
    Result.w = _mm256_mul_ps(Result.w, OneOverLen);
    
    return(Result);
}

#endif //JOY_AVX

#endif
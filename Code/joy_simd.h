#ifndef JOY_SIMD_H
#define JOY_SIMD_H

#include <xmmintrin.h>
#include <emmintrin.h>

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

struct i32_4x{
    __m128i e;
};

struct f32_4x{
    __m128 e;
};

struct v2_4x{
    union{
        struct{
            
            __m128 x;
            __m128 y;
        };
        
        __m128 e[2];
    };
};

struct v3_4x{
    union{
        struct{
            __m128 x;
            __m128 y;
            __m128 z;
        };
        
        __m128 e[3];
    };
};

struct v4_4x{
    union{
        struct{
            __m128 x;
            __m128 y;
            __m128 z;
            __m128 w;
        };
        
        __m128 e[4];
    };
    
};

struct m44_4x{
    union{
        struct{
            __m128 m00;
            __m128 m01;
            __m128 m02;
            __m128 m03;
            
            __m128 m10;
            __m128 m11;
            __m128 m12;
            __m128 m13;
            
            __m128 m20;
            __m128 m21;
            __m128 m22;
            __m128 m23;
            
            __m128 m30;
            __m128 m31;
            __m128 m32;
            __m128 m33;
        };
        
        __m128 m[16];
    };
};

// NOTE(Dima): Contstructors
inline i32_4x I32_4X(int A){
    i32_4x Result;
    
    Result.e = _mm_set1_epi32(A);
    
    return(Result);
}

inline i32_4x I32_4X(int A, int B, int C, int D){
    i32_4x Result;
    
    Result.e = _mm_setr_epi32(A, B, C, D);
    
    return(Result);
}


inline i32_4x I32_4X(__m128i Value){
    i32_4x Result;
    
    Result.e = Value;
    
    return(Result);
}


inline f32_4x F32_4X(f32 A){
    f32_4x Result;
    
    Result.e = _mm_setr_ps(A, A, A, A);
    
    return(Result);
}

inline f32_4x F32_4X(f32 A, f32 B, f32 C, f32 D){
    f32_4x Result;
    
    Result.e = _mm_setr_ps(A, B, C, D);
    
    return(Result);
}


inline f32_4x F32_4X(__m128 Value){
    f32_4x Result;
    
    Result.e = Value;
    
    return(Result);
}


inline v2_4x V2_4X(const v2& A,
                   const v2& B,
                   const v2& C,
                   const v2& D)
{
    v2_4x Result;
    
    Result.x = _mm_setr_ps(A.x, B.x, C.x, D.x);
    Result.y = _mm_setr_ps(A.y, B.y, C.y, D.y);
    
    return(Result);
}

inline v3_4x V3_4X(const v3& A,
                   const v3& B,
                   const v3& C,
                   const v3& D)
{
    v3_4x Result;
    
    Result.x = _mm_setr_ps(A.x, B.x, C.x, D.x);
    Result.y = _mm_setr_ps(A.y, B.y, C.y, D.y);
    Result.z = _mm_setr_ps(A.z, B.z, C.z, D.z);
    
    return(Result);
}

inline v3_4x V3_4X(const v3& A)
{
    v3_4x Result;
    
    Result.x = _mm_set1_ps(A.x);
    Result.y = _mm_set1_ps(A.y);
    Result.z = _mm_set1_ps(A.z);
    
    return(Result);
}

inline v4_4x V4_4X(const v4& A,
                   const v4& B,
                   const v4& C,
                   const v4& D)
{
    v4_4x Result;
    
    Result.x = _mm_setr_ps(A.x, B.x, C.x, D.x);
    Result.y = _mm_setr_ps(A.y, B.y, C.y, D.y);
    Result.z = _mm_setr_ps(A.z, B.z, C.z, D.z);
    Result.w = _mm_setr_ps(A.w, B.w, C.w, D.w);
    
    return(Result);
}

inline v4_4x V4_4X(const v4& A)
{
    v4_4x Result;
    
    Result.x = _mm_set1_ps(A.x);
    Result.y = _mm_set1_ps(A.y);
    Result.z = _mm_set1_ps(A.z);
    Result.w = _mm_set1_ps(A.w);
    
    return(Result);
}

inline v4_4x V4_4X(const quat& A)
{
    v4_4x Result;
    
    Result.x = _mm_set1_ps(A.x);
    Result.y = _mm_set1_ps(A.y);
    Result.z = _mm_set1_ps(A.z);
    Result.w = _mm_set1_ps(A.w);
    
    return(Result);
}

inline v4_4x V4_4X(const quat& A,
                   const quat& B,
                   const quat& C,
                   const quat& D)
{
    v4_4x Result;
    
    Result.x = _mm_setr_ps(A.x, B.x, C.x, D.x);
    Result.y = _mm_setr_ps(A.y, B.y, C.y, D.y);
    Result.z = _mm_setr_ps(A.z, B.z, C.z, D.z);
    Result.w = _mm_setr_ps(A.w, B.w, C.w, D.w);
    
    return(Result);
}

inline m44_4x M44_4X(const m44& A,
                     const m44& B,
                     const m44& C,
                     const m44& D)
{
    m44_4x Result;
    
    Result.m[0] = _mm_setr_ps(A.e[0], B.e[0], C.e[0], D.e[0]);
    Result.m[1] = _mm_setr_ps(A.e[1], B.e[1], C.e[1], D.e[1]);
    Result.m[2] = _mm_setr_ps(A.e[2], B.e[2], C.e[2], D.e[2]);
    Result.m[3] = _mm_setr_ps(A.e[3], B.e[3], C.e[3], D.e[3]);
    
    Result.m[4] = _mm_setr_ps(A.e[4], B.e[4], C.e[4], D.e[4]);
    Result.m[5] = _mm_setr_ps(A.e[5], B.e[5], C.e[5], D.e[5]);
    Result.m[6] = _mm_setr_ps(A.e[6], B.e[6], C.e[6], D.e[6]);
    Result.m[7] = _mm_setr_ps(A.e[7], B.e[7], C.e[7], D.e[7]);
    
    Result.m[8] = _mm_setr_ps(A.e[8], B.e[8], C.e[8], D.e[8]);
    Result.m[9] = _mm_setr_ps(A.e[9], B.e[9], C.e[9], D.e[9]);
    Result.m[10] = _mm_setr_ps(A.e[10], B.e[10], C.e[10], D.e[10]);
    Result.m[11] = _mm_setr_ps(A.e[11], B.e[11], C.e[11], D.e[11]);
    
    Result.m[12] = _mm_setr_ps(A.e[12], B.e[12], C.e[12], D.e[12]);
    Result.m[13] = _mm_setr_ps(A.e[13], B.e[13], C.e[13], D.e[13]);
    Result.m[14] = _mm_setr_ps(A.e[14], B.e[14], C.e[14], D.e[14]);
    Result.m[15] = _mm_setr_ps(A.e[15], B.e[15], C.e[15], D.e[15]);
    
    return(Result);
}

inline m44_4x TranslationMatrix_4X(const v3& Tran0,
                                   const v3& Tran1,
                                   const v3& Tran2,
                                   const v3& Tran3)
{
    m44_4x Result;
    
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 One = _mm_set1_ps(1.0f);
    
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
    
    Result.m[12] = _mm_setr_ps(Tran0.x, Tran1.x, Tran2.x, Tran3.x);
    Result.m[13] = _mm_setr_ps(Tran0.y, Tran1.y, Tran2.y, Tran3.y);
    Result.m[14] = _mm_setr_ps(Tran0.z, Tran1.z, Tran2.z, Tran3.z);
    Result.m[15] = One;
    
    return(Result);
}

inline m44_4x ScalingMatrix_4X(const v3& Scale0,
                               const v3& Scale1,
                               const v3& Scale2,
                               const v3& Scale3)
{
    m44_4x Result;
    
    __m128 Zero = _mm_set1_ps(0.0f);
    
    __m128 x = _mm_setr_ps(Scale0.x, Scale1.x, Scale2.x, Scale3.x);
    __m128 y = _mm_setr_ps(Scale0.y, Scale1.y, Scale2.y, Scale3.y);
    __m128 z = _mm_setr_ps(Scale0.z, Scale1.z, Scale2.z, Scale3.z);
    
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
    Result.m[15] = _mm_set1_ps(1.0f);
    
    return(Result);
}

inline m44_4x RotationMatrix_4X(const quat& Rot0,
                                const quat& Rot1,
                                const quat& Rot2,
                                const quat& Rot3)
{
    m44_4x Result;
    
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 One = _mm_set1_ps(1.0f);
    __m128 Two = _mm_set1_ps(2.0f);
    
    __m128 x = _mm_setr_ps(Rot0.x, Rot1.x, Rot2.x, Rot3.x);
    __m128 y = _mm_setr_ps(Rot0.y, Rot1.y, Rot2.y, Rot3.y);
    __m128 z = _mm_setr_ps(Rot0.z, Rot1.z, Rot2.z, Rot3.z);
    __m128 w = _mm_setr_ps(Rot0.w, Rot1.w, Rot2.w, Rot3.w);
    
    __m128 xx = _mm_mul_ps(x, x);
    __m128 yy = _mm_mul_ps(y, y);
    __m128 zz = _mm_mul_ps(z, z);
    
    __m128 xy = _mm_mul_ps(x, y);
    __m128 zw = _mm_mul_ps(z, w);
    __m128 xz = _mm_mul_ps(x, z);
    __m128 yw = _mm_mul_ps(y, w);
    __m128 yz = _mm_mul_ps(y, z);
    __m128 xw = _mm_mul_ps(x, w);
    
    Result.m[0] = _mm_sub_ps(One, _mm_mul_ps(Two, _mm_add_ps(yy, zz)));
    Result.m[1] = _mm_mul_ps(Two, _mm_add_ps(xy, zw));
    Result.m[2] = _mm_mul_ps(Two, _mm_sub_ps(xz, yw));
    Result.m[3] = Zero;
    
    Result.m[4] = _mm_mul_ps(Two, _mm_sub_ps(xy, zw));
    Result.m[5] = _mm_sub_ps(One, _mm_mul_ps(Two, _mm_add_ps(xx, zz)));
    Result.m[6] = _mm_mul_ps(Two, _mm_add_ps(yz, xw));
    Result.m[7] = Zero;
    
    Result.m[8] = _mm_mul_ps(Two, _mm_add_ps(xz, yw));
    Result.m[9] = _mm_mul_ps(Two, _mm_sub_ps(yz, xw));
    Result.m[10] = _mm_sub_ps(One, _mm_mul_ps(Two, _mm_add_ps(xx, yy)));
    Result.m[11] = Zero;
    
    Result.m[12] = Zero;
    Result.m[13] = Zero;
    Result.m[14] = Zero;
    Result.m[15] = One;
    
    return(Result);
}


// NOTE(Dima): Store
inline void I32_4X_Store(i32_4x ToStore,
                         int* A, int* B,
                         int* C, int* D)
{
    *A = MMI(ToStore.e, 0);
    *B = MMI(ToStore.e, 1);
    *C = MMI(ToStore.e, 2);
    *D = MMI(ToStore.e, 3);
}

inline void F32_4X_Store(f32_4x ToStore,
                         f32* A, f32* B,
                         f32* C, f32* D)
{
    *A = MM(ToStore.e, 0);
    *B = MM(ToStore.e, 1);
    *C = MM(ToStore.e, 2);
    *D = MM(ToStore.e, 3);
}

inline void V3_4X_Store(const v3_4x& Ref, 
                        v3* A,
                        v3* B,
                        v3* C,
                        v3* D)
{
    A->x = MM(Ref.x, 0);
    A->y = MM(Ref.y, 0);
    A->z = MM(Ref.z, 0);
    
    B->x = MM(Ref.x, 1);
    B->y = MM(Ref.y, 1);
    B->z = MM(Ref.z, 1);
    
    C->x = MM(Ref.x, 2);
    C->y = MM(Ref.y, 2);
    C->z = MM(Ref.z, 2);
    
    D->x = MM(Ref.x, 3);
    D->y = MM(Ref.y, 3);
    D->z = MM(Ref.z, 3);
}

inline void V4_4X_Store(const v4_4x& Ref, 
                        v4* A,
                        v4* B,
                        v4* C,
                        v4* D)
{
    A->x = MM(Ref.x, 0);
    A->y = MM(Ref.y, 0);
    A->z = MM(Ref.z, 0);
    A->w = MM(Ref.w, 0);
    
    B->x = MM(Ref.x, 1);
    B->y = MM(Ref.y, 1);
    B->z = MM(Ref.z, 1);
    B->w = MM(Ref.w, 1);
    
    C->x = MM(Ref.x, 2);
    C->y = MM(Ref.y, 2);
    C->z = MM(Ref.z, 2);
    C->w = MM(Ref.w, 2);
    
    D->x = MM(Ref.x, 3);
    D->y = MM(Ref.y, 3);
    D->z = MM(Ref.z, 3);
    D->w = MM(Ref.w, 3);
}

inline void V4_4X_Store(const v4_4x& Ref, 
                        quat* A,
                        quat* B,
                        quat* C,
                        quat* D)
{
    A->x = MM(Ref.x, 0);
    A->y = MM(Ref.y, 0);
    A->z = MM(Ref.z, 0);
    A->w = MM(Ref.w, 0);
    
    B->x = MM(Ref.x, 1);
    B->y = MM(Ref.y, 1);
    B->z = MM(Ref.z, 1);
    B->w = MM(Ref.w, 1);
    
    C->x = MM(Ref.x, 2);
    C->y = MM(Ref.y, 2);
    C->z = MM(Ref.z, 2);
    C->w = MM(Ref.w, 2);
    
    D->x = MM(Ref.x, 3);
    D->y = MM(Ref.y, 3);
    D->z = MM(Ref.z, 3);
    D->w = MM(Ref.w, 3);
}

#define M44_STORE_INDEX(i)\
A->e[i] = MM(Ref.m[i], 0);\
B->e[i] = MM(Ref.m[i], 1);\
C->e[i] = MM(Ref.m[i], 2);\
D->e[i] = MM(Ref.m[i], 3);

inline void M44_4X_Store(const m44_4x& Ref,
                         m44* A,
                         m44* B,
                         m44* C,
                         m44* D)
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

inline void I32_4X_StoreConditional(i32_4x ToStore,
                                    int MovedMask,
                                    int* StoreTo[4])
{
    for(int i = 0; i < 4; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            *StoreTo[i] = MMI(ToStore.e, i);
        }
    }
}

inline void F32_4X_StoreConditional(f32_4x ToStore,
                                    int MovedMask,
                                    f32* StoreTo[4])
{
    for(int i = 0; i < 4; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            *StoreTo[i] = MM(ToStore.e, i);
        }
    }
}

inline void V3_4X_StoreConditional(const v3_4x& Ref,
                                   int MovedMask,
                                   v3* StoreTo[4])
{
    for(int i = 0; i < 4; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM(Ref.e[0], i);
            StoreTo[i]->y = MM(Ref.e[1], i);
            StoreTo[i]->z = MM(Ref.e[2], i);
        }
    }
}

inline void V4_4X_StoreConditional(const v4_4x& Ref,
                                   int MovedMask,
                                   v4* StoreTo[4])
{
    for(int i = 0; i < 4; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM(Ref.e[0], i);
            StoreTo[i]->y = MM(Ref.e[1], i);
            StoreTo[i]->z = MM(Ref.e[2], i);
            StoreTo[i]->w = MM(Ref.e[3], i);
        }
    }
}

inline void V4_4X_StoreConditional(const v4_4x& Ref,
                                   int MovedMask,
                                   quat* StoreTo[4])
{
    for(int i = 0; i < 4; i++){
        if(StoreTo[i] && (MovedMask & (1 << i))){
            StoreTo[i]->x = MM(Ref.e[0], i);
            StoreTo[i]->y = MM(Ref.e[1], i);
            StoreTo[i]->z = MM(Ref.e[2], i);
            StoreTo[i]->w = MM(Ref.e[3], i);
        }
    }
}

// NOTE(Dima): Boolean operations

inline f32_4x IsTrue_4X(i32_4x A){
    __m128 Zero = _mm_set1_ps(0.0f);
    
    f32_4x Result;
    Result.e = _mm_cmpneq_ps(_mm_cvtepi32_ps(A.e), Zero);
    
    return(Result);
}

inline f32_4x IsEqual_4X(i32_4x A, i32_4x EqTo){
    f32_4x Result;
    
    Result.e = _mm_castsi128_ps(_mm_cmpeq_epi32(A.e, EqTo.e));
    
    return(Result);
}

inline f32_4x GreaterThan_4X(f32_4x A, f32_4x B){
    f32_4x Result;
    
    Result.e = _mm_cmpgt_ps(A.e, B.e);
    
    return(Result);
}

inline f32_4x GreaterEqThan_4X(f32_4x A, f32_4x B){
    f32_4x Result;
    
    Result.e = _mm_cmpge_ps(A.e, B.e);
    
    return(Result);
}

// NOTE(Dima): Addition
inline f32_4x operator+(const f32_4x& A, const f32_4x& B){
    f32_4x Result;
    Result.e = _mm_add_ps(A.e, B.e);
    return(Result);
}

inline v2_4x operator+(const v2_4x& A, const v2_4x& B){
    v2_4x Result;
    
    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    
    return(Result);
}


inline v3_4x operator+(const v3_4x& A, const v3_4x& B){
    v3_4x Result;
    
    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    Result.z = _mm_add_ps(A.z, B.z);
    
    return(Result);
}


inline v4_4x operator+(const v4_4x& A, const v4_4x& B){
    v4_4x Result;
    
    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    Result.z = _mm_add_ps(A.z, B.z);
    Result.w = _mm_add_ps(A.w, B.w);
    
    return(Result);
}

// NOTE(Dima): Add equal
inline f32_4x& operator+=(f32_4x& A, const f32_4x& B){
    A.e = _mm_add_ps(A.e, B.e);
    return(A);
}

inline v2_4x& operator+=(v2_4x& A, const v2_4x& B){
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    
    return(A);
}

inline v3_4x& operator+=(v3_4x& A, const v3_4x& B){
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    A.z = _mm_add_ps(A.z, B.z);
    
    return(A);
}

inline v4_4x& operator+=(v4_4x& A, const v4_4x& B){
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    A.z = _mm_add_ps(A.z, B.z);
    A.w = _mm_add_ps(A.w, B.w);
    
    return(A);
}

// NOTE(Dima): Subtraction
inline f32_4x operator-(const f32_4x& A, const f32_4x& B){
    f32_4x Result;
    Result.e = _mm_sub_ps(A.e, B.e);
    return(Result);
}

inline v2_4x operator-(const v2_4x& A, const v2_4x& B){
    v2_4x Result;
    
    Result.x = _mm_sub_ps(A.x, B.x);
    Result.y = _mm_sub_ps(A.y, B.y);
    
    return(Result);
}


inline v3_4x operator-(const v3_4x& A, const v3_4x& B){
    v3_4x Result;
    
    Result.x = _mm_sub_ps(A.x, B.x);
    Result.y = _mm_sub_ps(A.y, B.y);
    Result.z = _mm_sub_ps(A.z, B.z);
    
    return(Result);
}


inline v4_4x operator-(const v4_4x& A, const v4_4x& B){
    v4_4x Result;
    
    Result.x = _mm_sub_ps(A.x, B.x);
    Result.y = _mm_sub_ps(A.y, B.y);
    Result.z = _mm_sub_ps(A.z, B.z);
    Result.w = _mm_sub_ps(A.w, B.w);
    
    return(Result);
}

// NOTE(Dima): Multiplication
inline f32_4x operator*(const f32_4x& A, const f32_4x& B){
    f32_4x Result;
    Result.e = _mm_mul_ps(A.e, B.e);
    return(Result);
}

inline v2_4x operator*(const v2_4x& A, const v2_4x& B){
    v2_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.x);
    Result.y = _mm_mul_ps(A.y, B.y);
    
    return(Result);
}


inline v3_4x operator*(const v3_4x& A, const v3_4x& B){
    v3_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.x);
    Result.y = _mm_mul_ps(A.y, B.y);
    Result.z = _mm_mul_ps(A.z, B.z);
    
    return(Result);
}


inline v4_4x operator*(const v4_4x& A, const v4_4x& B){
    v4_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.x);
    Result.y = _mm_mul_ps(A.y, B.y);
    Result.z = _mm_mul_ps(A.z, B.z);
    Result.w = _mm_mul_ps(A.w, B.w);
    
    return(Result);
}

inline m44_4x operator*(const m44_4x& A, const m44_4x& B){
    m44_4x Result;
    
    // NOTE(Dima): First row
    Result.m[0] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[0], B.m[0]),
                                        _mm_mul_ps(A.m[1], B.m[4])),
                             _mm_add_ps(_mm_mul_ps(A.m[2], B.m[8]),
                                        _mm_mul_ps(A.m[3], B.m[12])));
    
    Result.m[1] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[0], B.m[1]),
                                        _mm_mul_ps(A.m[1], B.m[5])),
                             _mm_add_ps(_mm_mul_ps(A.m[2], B.m[9]),
                                        _mm_mul_ps(A.m[3], B.m[13])));
    
    Result.m[2] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[0], B.m[2]),
                                        _mm_mul_ps(A.m[1], B.m[6])),
                             _mm_add_ps(_mm_mul_ps(A.m[2], B.m[10]),
                                        _mm_mul_ps(A.m[3], B.m[14])));
    
    Result.m[3] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[0], B.m[3]),
                                        _mm_mul_ps(A.m[1], B.m[7])),
                             _mm_add_ps(_mm_mul_ps(A.m[2], B.m[11]),
                                        _mm_mul_ps(A.m[3], B.m[15])));
    
    // NOTE(Dima): Second row
    Result.m[4] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[4], B.m[0]),
                                        _mm_mul_ps(A.m[5], B.m[4])),
                             _mm_add_ps(_mm_mul_ps(A.m[6], B.m[8]),
                                        _mm_mul_ps(A.m[7], B.m[12])));
    
    Result.m[5] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[4], B.m[1]),
                                        _mm_mul_ps(A.m[5], B.m[5])),
                             _mm_add_ps(_mm_mul_ps(A.m[6], B.m[9]),
                                        _mm_mul_ps(A.m[7], B.m[13])));
    
    Result.m[6] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[4], B.m[2]),
                                        _mm_mul_ps(A.m[5], B.m[6])),
                             _mm_add_ps(_mm_mul_ps(A.m[6], B.m[10]),
                                        _mm_mul_ps(A.m[7], B.m[14])));
    
    Result.m[7] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[4], B.m[3]),
                                        _mm_mul_ps(A.m[5], B.m[7])),
                             _mm_add_ps(_mm_mul_ps(A.m[6], B.m[11]),
                                        _mm_mul_ps(A.m[7], B.m[15])));
    
    
    // NOTE(Dima): Third row
    Result.m[8] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[8], B.m[0]),
                                        _mm_mul_ps(A.m[9], B.m[4])),
                             _mm_add_ps(_mm_mul_ps(A.m[10], B.m[8]),
                                        _mm_mul_ps(A.m[11], B.m[12])));
    
    Result.m[9] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[8], B.m[1]),
                                        _mm_mul_ps(A.m[9], B.m[5])),
                             _mm_add_ps(_mm_mul_ps(A.m[10], B.m[9]),
                                        _mm_mul_ps(A.m[11], B.m[13])));
    
    Result.m[10] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[8], B.m[2]),
                                         _mm_mul_ps(A.m[9], B.m[6])),
                              _mm_add_ps(_mm_mul_ps(A.m[10], B.m[10]),
                                         _mm_mul_ps(A.m[11], B.m[14])));
    
    Result.m[11] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[8], B.m[3]),
                                         _mm_mul_ps(A.m[9], B.m[7])),
                              _mm_add_ps(_mm_mul_ps(A.m[10], B.m[11]),
                                         _mm_mul_ps(A.m[11], B.m[15])));
    
    
    // NOTE(Dima): Fourth row
    Result.m[12] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[12], B.m[0]),
                                         _mm_mul_ps(A.m[13], B.m[4])),
                              _mm_add_ps(_mm_mul_ps(A.m[14], B.m[8]),
                                         _mm_mul_ps(A.m[15], B.m[12])));
    
    Result.m[13] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[12], B.m[1]),
                                         _mm_mul_ps(A.m[13], B.m[5])),
                              _mm_add_ps(_mm_mul_ps(A.m[14], B.m[9]),
                                         _mm_mul_ps(A.m[15], B.m[13])));
    
    Result.m[14] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[12], B.m[2]),
                                         _mm_mul_ps(A.m[13], B.m[6])),
                              _mm_add_ps(_mm_mul_ps(A.m[14], B.m[10]),
                                         _mm_mul_ps(A.m[15], B.m[14])));
    
    Result.m[15] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.m[12], B.m[3]),
                                         _mm_mul_ps(A.m[13], B.m[7])),
                              _mm_add_ps(_mm_mul_ps(A.m[14], B.m[11]),
                                         _mm_mul_ps(A.m[15], B.m[15])));
    
    return(Result);
}

// NOTE(Dima): Multiplication by a scalar
inline v2_4x operator*(const v2_4x& A, const f32_4x& B){
    v2_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.e);
    Result.y = _mm_mul_ps(A.y, B.e);
    
    return(Result);
}


inline v3_4x operator*(const v3_4x& A, const f32_4x& B){
    v3_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.e);
    Result.y = _mm_mul_ps(A.y, B.e);
    Result.z = _mm_mul_ps(A.z, B.e);
    
    return(Result);
}


inline v4_4x operator*(const v4_4x& A, const f32_4x& B){
    v4_4x Result;
    
    Result.x = _mm_mul_ps(A.x, B.e);
    Result.y = _mm_mul_ps(A.y, B.e);
    Result.z = _mm_mul_ps(A.z, B.e);
    Result.w = _mm_mul_ps(A.w, B.e);
    
    return(Result);
}

// NOTE(Dima): Dot products
inline f32_4x Dot_4X(const v2_4x& A, const v2_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_mul_ps(A.x, B.x),
                          _mm_mul_ps(A.y, B.y));
    
    return(Result);
}

inline f32_4x Dot_4X(const v3_4x& A, const v3_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, B.x),
                                     _mm_mul_ps(A.y, B.y)),
                          _mm_mul_ps(A.z, B.z));
    
    return(Result);
}

inline f32_4x Dot_4X(const v4_4x& A, const v4_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, B.x),
                                     _mm_mul_ps(A.y, B.y)),
                          _mm_add_ps(_mm_mul_ps(A.z, B.z),
                                     _mm_mul_ps(A.w, B.w)));
    
    return(Result);
}



// NOTE(Dima): Normalize ops
inline v2_4x Normalize_4X(const v2_4x& A){
    __m128 SqLen = _mm_add_ps(_mm_mul_ps(A.x, A.x),
                              _mm_mul_ps(A.y, A.y));
    
    __m128 OneOverLen = _mm_rsqrt_ps(SqLen);
    
    v2_4x Result;
    
    Result.x = _mm_mul_ps(A.x, OneOverLen);
    Result.y = _mm_mul_ps(A.y, OneOverLen);
    
    return(Result);
}

inline v3_4x Normalize_4X(const v3_4x& A){
    __m128 SqLen = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, A.x),
                                         _mm_mul_ps(A.y, A.y)),
                              _mm_mul_ps(A.z, A.z));
    
    __m128 OneOverLen = _mm_rsqrt_ps(SqLen);
    
    v3_4x Result;
    
    Result.x = _mm_mul_ps(A.x, OneOverLen);
    Result.y = _mm_mul_ps(A.y, OneOverLen);
    Result.z = _mm_mul_ps(A.z, OneOverLen);
    
    return(Result);
}

inline v4_4x Normalize_4X(const v4_4x& A){
    __m128 SqLen = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, A.x),
                                         _mm_mul_ps(A.y, A.y)),
                              _mm_add_ps(_mm_mul_ps(A.z, A.z),
                                         _mm_mul_ps(A.w, A.w)));
    
    __m128 OneOverLen = _mm_rsqrt_ps(SqLen);
    
    v4_4x Result;
    
    Result.x = _mm_mul_ps(A.x, OneOverLen);
    Result.y = _mm_mul_ps(A.y, OneOverLen);
    Result.z = _mm_mul_ps(A.z, OneOverLen);
    Result.w = _mm_mul_ps(A.w, OneOverLen);
    
    return(Result);
}

// NOTE(Dima): 
inline __m128 ConditionalCombineInternal(__m128 Mask, __m128 A, __m128 B)
{
    __m128 Result = _mm_or_ps(_mm_and_ps(Mask, A),
                              _mm_andnot_ps(Mask, B));
    
    return(Result);
}

inline f32_4x ConditionalCombine(f32_4x Mask,
                                 f32_4x A,
                                 f32_4x B)
{
    f32_4x Result;
    
    Result.e = _mm_or_ps(_mm_and_ps(Mask.e, A.e),
                         _mm_andnot_ps(Mask.e, B.e));
    
    return(Result);
}

inline v2_4x ConditionalCombine(f32_4x Mask, 
                                const v2_4x& A, 
                                const v2_4x& B)
{
    v2_4x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    
    return(Result);
}

inline v3_4x ConditionalCombine(f32_4x Mask, 
                                const v3_4x& A, 
                                const v3_4x& B)
{
    v3_4x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    Result.z = ConditionalCombineInternal(Mask.e, A.z, B.z);
    
    return(Result);
}

inline v4_4x ConditionalCombine(f32_4x Mask, 
                                const v4_4x& A, 
                                const v4_4x& B)
{
    v4_4x Result;
    
    Result.x = ConditionalCombineInternal(Mask.e, A.x, B.x);
    Result.y = ConditionalCombineInternal(Mask.e, A.y, B.y);
    Result.z = ConditionalCombineInternal(Mask.e, A.z, B.z);
    Result.w = ConditionalCombineInternal(Mask.e, A.w, B.w);
    
    return(Result);
}

inline m44_4x ConditionalCombine(f32_4x Mask,
                                 const m44_4x& A,
                                 const m44_4x& B)
{
    m44_4x Result;
    
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
inline v2_4x Lerp_4X(const v2_4x& A,
                     const v2_4x& B,
                     const f32_4x& t)
{
    v2_4x Result;
    
    Result.x = _mm_add_ps(A.x, _mm_mul_ps(_mm_sub_ps(B.x, A.x), t.e));
    Result.y = _mm_add_ps(A.y, _mm_mul_ps(_mm_sub_ps(B.y, A.y), t.e));
    
    return(Result);
}

inline v3_4x Lerp_4X(const v3_4x& A,
                     const v3_4x& B,
                     const f32_4x& t)
{
    v3_4x Result;
    
    Result.x = _mm_add_ps(A.x, _mm_mul_ps(_mm_sub_ps(B.x, A.x), t.e));
    Result.y = _mm_add_ps(A.y, _mm_mul_ps(_mm_sub_ps(B.y, A.y), t.e));
    Result.z = _mm_add_ps(A.z, _mm_mul_ps(_mm_sub_ps(B.z, A.z), t.e));
    
    return(Result);
}

inline v4_4x Lerp_4X(const v4_4x& A,
                     const v4_4x& B,
                     const f32_4x& t)
{
    v4_4x Result;
    
    Result.x = _mm_add_ps(A.x, _mm_mul_ps(_mm_sub_ps(B.x, A.x), t.e));
    Result.y = _mm_add_ps(A.y, _mm_mul_ps(_mm_sub_ps(B.y, A.y), t.e));
    Result.z = _mm_add_ps(A.z, _mm_mul_ps(_mm_sub_ps(B.z, A.z), t.e));
    Result.w = _mm_add_ps(A.w, _mm_mul_ps(_mm_sub_ps(B.w, A.w), t.e));
    
    return(Result);
}

inline v4_4x LerpQuat_4X(const v4_4x& A,
                         const v4_4x& B,
                         const f32_4x& t)
{
    v4_4x Result;
    
    __m128 Zero = _mm_set1_ps(0.0f);
    __m128 T = t.e;
    __m128 OneMinusT = _mm_sub_ps(_mm_set1_ps(1.0f), t.e);
    
    f32_4x DotRes = Dot_4X(A, B);
    __m128 ResMask = _mm_cmplt_ps(DotRes.e, Zero);
    
    // NOTE(Dima): Building up needed B
    v4_4x NegatedB;
    NegatedB.x = _mm_sub_ps(Zero, B.x);
    NegatedB.y = _mm_sub_ps(Zero, B.y);
    NegatedB.z = _mm_sub_ps(Zero, B.z);
    NegatedB.w = _mm_sub_ps(Zero, B.w);
    
    v4_4x NewB;
    NewB.x = ConditionalCombineInternal(ResMask, NegatedB.x, B.x);
    NewB.y = ConditionalCombineInternal(ResMask, NegatedB.y, B.y);
    NewB.z = ConditionalCombineInternal(ResMask, NegatedB.z, B.z);
    NewB.w = ConditionalCombineInternal(ResMask, NegatedB.w, B.w);
    
    // NOTE(Dima): Lerping
    Result.x = _mm_add_ps(_mm_mul_ps(A.x, OneMinusT), _mm_mul_ps(NewB.x, T));
    Result.y = _mm_add_ps(_mm_mul_ps(A.y, OneMinusT), _mm_mul_ps(NewB.y, T));
    Result.z = _mm_add_ps(_mm_mul_ps(A.z, OneMinusT), _mm_mul_ps(NewB.z, T));
    Result.w = _mm_add_ps(_mm_mul_ps(A.w, OneMinusT), _mm_mul_ps(NewB.w, T));
    
    // NOTE(Dima): Normalizing
    __m128 SqLen = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Result.x, Result.x),
                                         _mm_mul_ps(Result.y, Result.y)),
                              _mm_add_ps(_mm_mul_ps(Result.z, Result.z),
                                         _mm_mul_ps(Result.w, Result.w)));
    
    __m128 OneOverLen = _mm_rsqrt_ps(SqLen);
    
    Result.x = _mm_mul_ps(Result.x, OneOverLen);
    Result.y = _mm_mul_ps(Result.y, OneOverLen);
    Result.z = _mm_mul_ps(Result.z, OneOverLen);
    Result.w = _mm_mul_ps(Result.w, OneOverLen);
    
    return(Result);
}

// NOTE(Dima): Other functions
inline f32_4x SignNotZero_4X(const f32_4x& A){
    __m128 Mask = _mm_cmpgt_ps(A.e, _mm_set1_ps(0.0f));
    
    f32_4x Result;
    Result.e = _mm_or_ps(_mm_and_ps(Mask, _mm_set1_ps(1.0f)),
                         _mm_andnot_ps(Mask, _mm_set1_ps(-1.0f)));
    
    return(Result);
}


#endif
#ifndef JOY_SIMD_H
#define JOY_SIMD_H

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

struct f32_4x{
    __m128 e;
};

struct v2_4x{
    __m128 x;
    __m128 y;
};

struct v3_4x{
    __m128 x;
    __m128 y;
    __m128 z;
};

struct v4_4x{
    __m128 x;
    __m128 y;
    __m128 z;
    __m128 w;
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
inline f32_4x F32_4x(f32 A){
    f32_4x Result;
    
    Result.e = _mm_setr_ps(A, A, A, A);
    
    return(Result);
}

inline f32_4x F32_4x(f32 A, f32 B, f32 C, f32 D){
    f32_4x Result;
    
    Result.e = _mm_setr_ps(A, B, C, D);
    
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

// NOTE(Dima): Store
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
D->e[i] = MM(Ref.m[i], 3);\


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
inline f32_4x Dot(const v2_4x& A, const v2_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_mul_ps(A.x, B.x),
                          _mm_mul_ps(A.y, B.y));
    
    return(Result);
}

inline f32_4x Dot(const v3_4x& A, const v3_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, B.x),
                                     _mm_mul_ps(A.y, B.y)),
                          _mm_mul_ps(A.z, B.z));
    
    return(Result);
}

inline f32_4x Dot(const v4_4x& A, const v4_4x& B){
    f32_4x Result;
    
    Result.e = _mm_add_ps(_mm_add_ps(_mm_mul_ps(A.x, B.x),
                                     _mm_mul_ps(A.y, B.y)),
                          _mm_add_ps(_mm_mul_ps(A.z, B.z),
                                     _mm_mul_ps(A.w, B.w)));
    
    return(Result);
}



// NOTE(Dima): Normalize ops
inline v2_4x Normalize(const v2_4x& A){
    __m128 SqLen = _mm_add_ps(_mm_mul_ps(A.x, A.x),
                              _mm_mul_ps(A.y, A.y));
    
    __m128 OneOverLen = _mm_rsqrt_ps(SqLen);
    
    v2_4x Result;
    
    Result.x = _mm_mul_ps(A.x, OneOverLen);
    Result.y = _mm_mul_ps(A.y, OneOverLen);
    
    return(Result);
}

inline v3_4x Normalize(const v3_4x& A){
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

inline v4_4x Normalize(const v4_4x& A){
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

// NOTE(Dima): Other functions
inline f32_4x SignNotZero(const f32_4x& A){
    __m128 Mask = _mm_cmpgt_ps(A.e, _mm_set1_ps(0.0f));
    
    f32_4x Result;
    Result.e = _mm_or_ps(_mm_and_ps(Mask, _mm_set1_ps(1.0f)),
                         _mm_andnot_ps(Mask, _mm_set1_ps(-1.0f)));
    
    return(Result);
}


#endif
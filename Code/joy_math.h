#ifndef JOY_MATH_H
#define JOY_MATH_H

#include <cmath>
#include <cstdint>

#define JOY_DEG2RAD 0.0174532925f
#define JOY_RAD2DEG 57.2958f

#define JOY_PI 3.14159265359f
#define JOY_TWO_PI 6.28318530718f
#define JOY_PI_OVER_TWO 1.57079632679f

#define JOY_ONE_OVER_255 0.00392156862f;

#define JOY_ENABLE_SIMD_MATH 1

#if JOY_ENABLE_SIMD_MATH
#include <xmmintrin.h>
#endif

#define JOY_MATH_MIN(a, b) ((a) > (b) ? (b) : (a))
#define JOY_MATH_MAX(a, b) ((a) > (b) ? (a) : (b))

//NOTE(dima): Structures
typedef union v2 {
	struct {
		float x;
		float y;
	};
    
	float E[2];
} v2;

typedef union v3 {
	struct {
		float x;
		float y;
		float z;
	};
    
	struct {
		float r;
		float g;
		float b;
	};
    
	struct {
		float A;
		float B;
		float C;
	};
    
	float E[3];
} v3;

typedef union v4 {
	struct {
		union {
			struct {
				float x;
				float y;
				float z;
			};
            
			v3 xyz;
		};
        
		float w;
	};
    
	struct {
		union {
			struct {
				float r;
				float g;
				float b;
			};
            
			v3 rgb;
		};
        
		float a;
	};
    
	struct {
		union {
			struct {
				float A;
				float B;
				float C;
			};
			v3 ABC;
			v3 N;
		};
        
		float D;
	};
    
	float E[4];
} v4;

typedef struct rc2{
    v2 Min;
    v2 Max;
} rc2;

typedef union m33 {
    float E[9];
    float E2[3][3];
    v3 Rows[3];
} m33;

typedef union m44 {
	float E[16];
    float E2[4][4];
	v4 Rows[4];
#if JOY_ENABLE_SIMD_MATH
    __m128 mmRows[4];
#endif
} m44;

typedef union quat {
	struct {
		struct {
			union {
				struct{
					float x, y, z;
				};
                
				v3 xyz;
			};
		};
        
		float w;
	};
    
	v4 xyzw;
} quat;

//NOTE(dima): Helper functions
inline float Sqrt(float Value) {
	float Result;
	Result = sqrtf(Value);
	return(Result);
}

inline float RSqrt(float Value) {
	float Result;
	Result = 1.0f / sqrtf(Value);
	return(Result);
}

inline float Floor(float Value) {
	float Result = floorf(Value);
	return(Result);
}

inline float Ceil(float Value) {
	float Result = ceilf(Value);
	return(Result);
}

inline float Sin(float Rad) {
	float Result = sinf(Rad);
	return(Result);
}

inline float Cos(float Rad) {
	float Result = cosf(Rad);
	return(Result);
}

inline float Tan(float Rad) {
	float Result = tanf(Rad);
	return(Result);
}

inline float ASin(float Value) {
	float Result = asinf(Value);
	return(Result);
}

inline float ACos(float Value) {
	float Result = acosf(Value);
	return(Result);
}

inline float ATan(float Value) {
	float Result = atan(Value);
	return(Result);
}

inline float ATan2(float Y, float X) {
	float Result = atan2f(Y, X);
	return(Result);
}

inline float Exp(float Value) {
	float Result = expf(Value);
	return(Result);
}

inline float Log(float Value) {
	float Result = logf(Value);
	return(Result);
}

inline float Pow(float a, float b) {
	float Result = powf(a, b);
	return(Result);
}

inline float Lerp(float a, float b, float t) {
	float Result = a + (b - a) * t;
    
	return(Result);
}

inline float Clamp01(float Val) {
	if (Val < 0.0f) {
		Val = 0.0f;
	}
    
	if (Val > 1.0f) {
		Val = 1.0f;
	}
    
	return(Val);
}

inline float Clamp(float Val, float Min, float Max) {
	if (Val < Min) {
		Val = Min;
	}
    
	if (Val > Max) {
		Val = Max;
	}
    
	return(Val);
}

inline int Clamp(int Val, int Min, int Max) {
	if (Val < Min) {
		Val = Min;
	}
    
	if (Val > Max) {
		Val = Max;
	}
    
	return(Val);
}

//NOTE(dima): Constructors
inline v2 V2(float Value){
	v2 Result;
    
	Result.x = Value;
	Result.y = Value;
    
	return(Result);
}

inline v2 V2(float x, float y) {
	v2 Result;
    
	Result.x = x;
	Result.y = y;
    
	return(Result);
}

inline v3 V3(v2 xy, float z) {
	v3 Result;
    
	Result.x = xy.x;
	Result.y = xy.y;
	Result.z = z;
    
	return(Result);
}

inline v3 V3(float x, float y, float z) {
	v3 Result;
    
	Result.x = x;
	Result.y = y;
	Result.z = z;
    
	return(Result);
}

inline v4 V4(float Value) {
	v4 Result;
	Result.x = Value;
	Result.y = Value;
	Result.z = Value;
	Result.w = Value;
	return(Result);
}

inline v4 V4(float x, float y, float z, float w) {
	v4 Result;
    
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;
    
	return(Result);
}

inline v4 V4(v3 InitVector, float w) {
	v4 Result;
	Result.x = InitVector.x;
	Result.y = InitVector.y;
	Result.z = InitVector.z;
	Result.w = w;
	return(Result);
}

inline quat QuatI(){
	quat Result;
    
	Result.x = 0.0f;
	Result.y = 0.0f;
	Result.z = 0.0f;
	Result.w = 1.0f;
    
	return(Result);
}

inline quat Quat(float x, float y, float z, float w){
	quat Result;
    
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;
    
	return(Result);
}

inline quat Quat(v3 Axis, float Angle){
	quat Result;
    
	float S = Sin(Angle);
	Result.x = Axis.x * S;
	Result.y = Axis.y * S;
	Result.z = Axis.z * S;
	Result.w = Angle * Cos(Angle);
    
	return(Result);
}

inline m33 M33I(){
    m33 Result;
    
    Result.Rows[0] = {1.0f, 0.0f, 0.0f};
    Result.Rows[1] = {0.0f, 1.0f, 0.0f};
    Result.Rows[2] = {0.0f, 0.0f, 1.0f};
    
    return(Result);
}

inline m44 M44I(){
    m44 Result;
    
    Result.Rows[0] = { 1.0f, 0.0f, 0.0f, 0.0f };
    Result.Rows[1] = { 0.0f, 1.0f, 0.0f, 0.0f };
    Result.Rows[2] = { 0.0f, 0.0f, 1.0f, 0.0f };
    Result.Rows[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    return(Result);
}

/*Dot operations*/
inline float Dot(v2 A, v2 B) {
    return A.x * B.x + A.y * B.y;
}

inline float Dot(v3 A, v3 B) {
    return A.x * B.x + A.y * B.y + A.z * B.z;
}

inline float Dot(v4 A, v4 B) {
    return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
}

inline float Dot(quat A, quat B){
    return A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
}

/*Cross product*/
inline float Cross(v2 A, v2 B) { 
    return A.x * B.y - B.x * A.y; 
}

inline v3 Cross(v3 A, v3 B) {
    v3 R;
    R.x = A.y * B.z - B.y * A.z;
    R.y = A.z * B.x - B.z * A.x;
    R.z = A.x * B.y - B.x * A.y;
    return(R);
}


/*Add operations*/
inline v2 Add(v2 A, v2 B) {
    A.x += B.x;
    A.y += B.y;
    
    return(A);
}

inline v3 Add(v3 A, v3 B) {
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
    
    return(A);
}

inline v4 Add(v4 A, v4 B) {
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
    A.w += B.w;
    
    return(A);
}

inline quat Add(quat A, quat B){
    A.x += B.x;
    A.y += B.y;
    A.z += B.z;
    A.w += B.w;
    
    return(A);
}

/*Subtract operations*/
inline v2 Sub(v2 A, v2 B) {
    A.x -= B.x;
    A.y -= B.y;
    
    return(A);
}

inline v3 Sub(v3 A, v3 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;
    
    return(A);
}

inline v4 Sub(v4 A, v4 B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;
    A.w -= B.w;
    
    return(A);
}

inline quat Sub(quat A, quat B) {
    A.x -= B.x;
    A.y -= B.y;
    A.z -= B.z;
    A.w -= B.w;
    
    return(A);
}

/*Multiply operations*/
inline v2 Mul(v2 A, float S) {
    A.x *= S;
    A.y *= S;
    return(A);
}

inline v3 Mul(v3 A, float S) {
    A.x *= S;
    A.y *= S;
    A.z *= S;
    return(A);
}

inline v4 Mul(v4 A, float S) {
    A.x *= S;
    A.y *= S;
    A.z *= S;
    A.w *= S;
    return(A);
}

inline quat Mul(quat A, quat B){
    quat R;
    
    R.w = A.w * B.w - A.x * B.x - A.y * B.y - A.z * B.z;
    R.x = A.w * B.x + A.x * B.w + A.y * B.z - A.z * B.y;
    R.y = A.w * B.y + A.y * B.w + A.z * B.x - A.x * B.z;
    R.z = A.w * B.z + A.z * B.w + A.x * B.y - A.y * B.x;
    
    return(R);
}

inline quat Mul(quat A, float S){
    A.x *= S;
    A.y *= S;
    A.z *= S;
    A.w *= S;
    
    return(A);
}


#if JOY_ENABLE_SIMD_MATH
inline __m128 MulVecMatSSE(__m128 V, const m44& M){
    
    __m128 vX = _mm_shuffle_ps(V, V, 0x00);
    __m128 vY = _mm_shuffle_ps(V, V, 0x55);
    __m128 vZ = _mm_shuffle_ps(V, V, 0xAA);
    __m128 vW = _mm_shuffle_ps(V, V, 0xFF);
    
    __m128 Result = _mm_mul_ps(vX, M.mmRows[0]);
    Result = _mm_add_ps(Result, _mm_mul_ps(vY, M.mmRows[1]));
    Result = _mm_add_ps(Result, _mm_mul_ps(vZ, M.mmRows[2]));
    Result = _mm_add_ps(Result, _mm_mul_ps(vW, M.mmRows[3]));
    
    return(Result);
}
#endif

inline m44 Mul(m44 A, m44 B) {
    m44 Result = {};
    
#if JOY_ENABLE_SIMD_MATH
    Result.mmRows[0] = MulVecMatSSE(A.mmRows[0], B);
    Result.mmRows[1] = MulVecMatSSE(A.mmRows[1], B);
    Result.mmRows[2] = MulVecMatSSE(A.mmRows[2], B);
    Result.mmRows[3] = MulVecMatSSE(A.mmRows[3], B);
#else
    Result.E[0] = A.E[0] * B.E[0] + A.E[1] * B.E[4] + A.E[2] * B.E[8] + A.E[3] * B.E[12];
    Result.E[1] = A.E[0] * B.E[1] + A.E[1] * B.E[5] + A.E[2] * B.E[9] + A.E[3] * B.E[13];
    Result.E[2] = A.E[0] * B.E[2] + A.E[1] * B.E[6] + A.E[2] * B.E[10] + A.E[3] * B.E[14];
    Result.E[3] = A.E[0] * B.E[3] + A.E[1] * B.E[7] + A.E[2] * B.E[11] + A.E[3] * B.E[15];
    
    Result.E[4] = A.E[4] * B.E[0] + A.E[5] * B.E[4] + A.E[6] * B.E[8] + A.E[7] * B.E[12];
    Result.E[5] = A.E[4] * B.E[1] + A.E[5] * B.E[5] + A.E[6] * B.E[9] + A.E[7] * B.E[13];
    Result.E[6] = A.E[4] * B.E[2] + A.E[5] * B.E[6] + A.E[6] * B.E[10] + A.E[7] * B.E[14];
    Result.E[7] = A.E[4] * B.E[3] + A.E[5] * B.E[7] + A.E[6] * B.E[11] + A.E[7] * B.E[15];
    
    Result.E[8] = A.E[8] * B.E[0] + A.E[9] * B.E[4] + A.E[10] * B.E[8] + A.E[11] * B.E[12];
    Result.E[9] = A.E[8] * B.E[1] + A.E[9] * B.E[5] + A.E[10] * B.E[9] + A.E[11] * B.E[13];
    Result.E[10] = A.E[8] * B.E[2] + A.E[9] * B.E[6] + A.E[10] * B.E[10] + A.E[11] * B.E[14];
    Result.E[11] = A.E[8] * B.E[3] + A.E[9] * B.E[7] + A.E[10] * B.E[11] + A.E[11] * B.E[15];
    
    Result.E[12] = A.E[12] * B.E[0] + A.E[13] * B.E[4] + A.E[14] * B.E[8] + A.E[15] * B.E[12];
    Result.E[13] = A.E[12] * B.E[1] + A.E[13] * B.E[5] + A.E[14] * B.E[9] + A.E[15] * B.E[13];
    Result.E[14] = A.E[12] * B.E[2] + A.E[13] * B.E[6] + A.E[14] * B.E[10] + A.E[15] * B.E[14];
    Result.E[15] = A.E[12] * B.E[3] + A.E[13] * B.E[7] + A.E[14] * B.E[11] + A.E[15] * B.E[15];
#endif
    
    return(Result);
}

inline v4 Mul(v4 V, m44 M) {
    v4 Result;
    
    Result.E[0] = V.E[0] * M.E[0] + V.E[1] * M.E[4] + V.E[2] * M.E[8] + V.E[3] * M.E[12];
    Result.E[1] = V.E[0] * M.E[1] + V.E[1] * M.E[5] + V.E[2] * M.E[9] + V.E[3] * M.E[13];
    Result.E[2] = V.E[0] * M.E[2] + V.E[1] * M.E[6] + V.E[2] * M.E[10] + V.E[3] * M.E[14];
    Result.E[3] = V.E[0] * M.E[3] + V.E[1] * M.E[7] + V.E[2] * M.E[11] + V.E[3] * M.E[15];
    
    return(Result);
}


/*Divide operations*/
inline v2 Div(v2 A, float S) {
    float OneOverS = 1.0f / S;
    
    A.x *= OneOverS;
    A.y *= OneOverS;
    
    return(A);
}

inline v3 Div(v3 A, float S) {
    float OneOverS = 1.0f / S;
    
    A.x *= OneOverS;
    A.y *= OneOverS;
    A.z *= OneOverS;
    
    return(A);
}

inline v4 Div(v4 A, float S) {
    float OneOverS = 1.0f / S;
    
    A.x *= OneOverS;
    A.y *= OneOverS;
    A.z *= OneOverS;
    A.w *= OneOverS;
    
    return(A);
}

inline quat Div(quat A, float S) {
    float OneOverS = 1.0f / S;
    
    A.x *= OneOverS;
    A.y *= OneOverS;
    A.z *= OneOverS;
    A.w *= OneOverS;
    
    return(A);
}

/*Hadamard product*/
inline v2 Hadamard(v2 A, v2 B) { return (V2(A.x * B.x, A.y * B.y)); }
inline v3 Hadamard(v3 A, v3 B) { return (V3(A.x * B.x, A.y * B.y, A.z * B.z)); }
inline v4 Hadamard(v4 A, v4 B) { return (V4(A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w)); }

/*Magnitude of the vector*/
inline float Magnitude(v2 A) { return(Sqrt(Dot(A, A))); }
inline float Magnitude(v3 A) { return(Sqrt(Dot(A, A))); }
inline float Magnitude(v4 A) { return(Sqrt(Dot(A, A))); }
inline float Magnitude(quat A) { return(Sqrt(Dot(A, A))); }

/*Squared magnitude*/
inline float SqMagnitude(v2 A) { return(Dot(A, A)); }
inline float SqMagnitude(v3 A) { return(Dot(A, A)); }
inline float SqMagnitude(v4 A) { return(Dot(A, A)); }
inline float SqMagnitude(quat A) { return(Dot(A, A)); }

/*v2 operator overloading*/
inline v2 operator+(v2 A) { return(A); }
inline v2 operator-(v2 A) { v2 R = { -A.x, -A.y }; return(R); }

inline v2 operator+(v2 A, v2 b) { return Add(A, b); }
inline v2 operator-(v2 A, v2 b) { return Sub(A, b); }

inline v2 operator*(v2 A, float S) { return Mul(A, S); }
inline v2 operator*(float S, v2 A) { return Mul(A, S); }
inline v2 operator/(v2 A, float S) { return Div(A, S); }

inline v2 operator*(v2 A, v2 b) { v2 R = { A.x * b.x, A.y * b.y }; return(R); }
inline v2 operator/(v2 A, v2 b) { v2 R = { A.x / b.x, A.y / b.y }; return(R); }

inline v2 &operator+=(v2& A, v2 b) { return(A = A + b); }
inline v2 &operator-=(v2& A, v2 b) { return(A = A - b); }
inline v2 &operator*=(v2& A, float S) { return(A = A * S); }
inline v2 &operator/=(v2& A, float S) { return(A = A / S); }

/*v3 operator overloading*/
inline v3 operator+(v3 A) { return(A); }
inline v3 operator-(v3 A) { v3 R = { -A.x, -A.y, -A.z }; return(R); }

inline v3 operator+(v3 A, v3 b) { return Add(A, b); }
inline v3 operator-(v3 A, v3 b) { return Sub(A, b); }

inline v3 operator*(v3 A, float S) { return Mul(A, S); }
inline v3 operator*(float S, v3 A) { return Mul(A, S); }
inline v3 operator/(v3 A, float S) { return Div(A, S); }

inline v3 operator*(v3 A, v3 b) { v3 R = { A.x * b.x, A.y * b.y, A.z * b.z }; return(R); }
inline v3 operator/(v3 A, v3 b) { v3 R = { A.x / b.x, A.y / b.y, A.z / b.z }; return(R); }

inline v3 &operator+=(v3& A, v3 b) { return(A = A + b); }
inline v3 &operator-=(v3& A, v3 b) { return(A = A - b); }
inline v3 &operator*=(v3& A, float S) { return(A = A * S); }
inline v3 &operator/=(v3& A, float S) { return(A = A / S); }

/*v4 operator overloading*/
inline v4 operator+(v4 A) { return(A); }
inline v4 operator-(v4 A) { v4 R = { -A.x, -A.y, -A.z, -A.w }; return(R); }

inline v4 operator+(v4 A, v4 B) { return Add(A, B); }
inline v4 operator-(v4 A, v4 B) { return Sub(A, B); }

inline v4 operator*(v4 A, float S) { return Mul(A, S); }
inline v4 operator*(float S, v4 A) { return Mul(A, S); }
inline v4 operator/(v4 A, float S) { return Div(A, S); }

inline v4 operator*(v4 A, v4 B) { v4 R = { A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w }; return(R); }
inline v4 operator/(v4 A, v4 B) { v4 R = { A.x / B.x, A.y / B.y, A.z / B.z, A.w / B.w }; return(R); }

inline v4 &operator+=(v4& A, v4 B) { return(A = A + B); }
inline v4 &operator-=(v4& A, v4 B) { return(A = A - B); }
inline v4 &operator*=(v4& A, float S) { return(A = A * S); }
inline v4 &operator/=(v4& A, float S) { return(A = A / S); }

/*quat operator overloading*/
inline quat operator+(quat A) { return(A); }
inline quat operator-(quat A) { return(A); }

inline quat operator+(quat A, quat B) { return Add(A, B); }
inline quat operator-(quat A, quat B) { return Sub(A, B); }

inline quat operator*(quat A, float S) { return Mul(A, S); }
inline quat operator*(float S, quat A) { return Mul(A, S); }
inline quat operator/(quat A, float S) { return Div(A, S); }

inline quat operator*(quat A, quat B) { return(Mul(A, B)); }

/*Matrix operator overloading*/
inline v4 operator*(v4 A, m44 B){
    v4 Result = Mul(A, B);
    
    return(Result);
}

inline m44 operator*(m44 A, m44 B){
    m44 Result = Mul(A, B);
    
    return(Result);
}

/*Normalization operations*/
inline v2 Normalize(v2 A) { return(Mul(A, RSqrt(Dot(A, A)))); }
inline v3 Normalize(v3 A) { return(Mul(A, RSqrt(Dot(A, A)))); }
inline v4 Normalize(v4 A) { return(Mul(A, RSqrt(Dot(A, A)))); }
inline quat Normalize(quat A) { return(Mul(A, RSqrt(Dot(A, A)))); }

/*Safe normalization operations*/
inline v2 NOZ(v2 A) { float SqMag = Dot(A, A); return((SqMag) < 0.0000001f ? V2(0.0f, 0.0f) : A * RSqrt(SqMag)); }
inline v3 NOZ(v3 A) { float SqMag = Dot(A, A); return((SqMag) < 0.0000001f ? V3(0.0f, 0.0f, 0.0f) : A * RSqrt(SqMag)); }
inline v4 NOZ(v4 A) { float SqMag = Dot(A, A); return((SqMag) < 0.0000001f ? V4(0.0f, 0.0f, 0.0f, 0.0f) : A * RSqrt(SqMag)); }

/*Lerp operations*/
inline v2 Lerp(v2 A, v2 B, float t) { return((1.0f - t) * A + B * t); }
inline v3 Lerp(v3 A, v3 B, float t) { return((1.0f - t) * A + B * t); }
inline v4 Lerp(v4 A, v4 B, float t) { return((1.0f - t) * A + B * t); }

/*Quaternion operations*/
inline quat Conjugate(quat Q){
    quat Result = {};
    
    Result.xyz = -Q.xyz;
    Result.w = Q.w;
    
    return(Result);
}

inline quat Inverse(quat Q){
    quat Result = Conjugate(Q) / Dot(Q, Q);
    
    return(Result);
}

inline quat Lerp(quat A, quat B, float t) {
    quat Result;
    
    
    
    return(Result);
}

inline quat Slerp(quat A, quat B, float t){
    quat Result;
    
    
    
    return(Result);
}

/*Conversions*/
inline m44 MatrixFromRows(v4 R1, v4 R2, v4 R3, v4 R4){
    m44 Result = {};
    
    Result.Rows[0] = R1;
    Result.Rows[1] = R2;
    Result.Rows[2] = R3;
    Result.Rows[3] = R4;
    
    return(Result);
}

inline m33 MatrixFromRows(v3 R1, v3 R2, v3 R3){
    m33 Result = {};
    
    Result.Rows[0] = R1;
    Result.Rows[1] = R2;
    Result.Rows[2] = R3;
    
    return(Result);
}

inline m33 QuatToM33(quat Q){
    m33 Res = {};
    
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    Res.E[0] = 1.0f - 2.0f * (y2 + z2);
    Res.E[1] = 2.0f * (xy + zw);
    Res.E[2] = 2.0f * (xz - yw);
    
    Res.E[3] = 2.0f * (xy - zw);
    Res.E[4] = 1.0f - 2.0f * (x2 + z2);
    Res.E[5] = 2.0f * (yz + xw);
    
    Res.E[6] = 2.0f * (xz + yw);
    Res.E[7] = 2.0f * (yz - xw);
    Res.E[8] = 1.0f - 2.0f * (x2 + y2);
    
    return(Res);
}

inline m44 QuatToM44(quat Q){
    m44 Res = {};
    
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    Res.E[0] = 1.0f - 2.0f * (y2 + z2);
    Res.E[1] = 2.0f * (xy + zw);
    Res.E[2] = 2.0f * (xz - yw);
    Res.E[3] = 0.0f;
    
    Res.E[4] = 2.0f * (xy - zw);
    Res.E[5] = 1.0f - 2.0f * (x2 + z2);
    Res.E[6] = 2.0f * (yz + xw);
    Res.E[7] = 0.0f;
    
    Res.E[8] = 2.0f * (xz + yw);
    Res.E[9] = 2.0f * (yz - xw);
    Res.E[10] = 1.0f - 2.0f * (x2 + y2);
    Res.E[11] = 0.0f;
    
    Res.E[12] = 0.0f;
    Res.E[13] = 0.0f;
    Res.E[14] = 0.0f;
    Res.E[15] = 1.0f;
    
    return(Res);
}

inline quat QuatFrom2DArray(float A[3][3]){
    quat Result;
    
    float Trace = A[0][0] + A[1][1] + A[2][2]; // I removed + 1.0f; see discussion with Ethan
    if( Trace > 0 ) {// I changed M_EPSILON to 0
        float S = 0.5f / sqrtf(Trace + 1.0f);
        Result.w = 0.25f / S;
        Result.x = ( A[2][1] - A[1][2] ) * S;
        Result.y = ( A[0][2] - A[2][0] ) * S;
        Result.z = ( A[1][0] - A[0][1] ) * S;
    } else {
        if ( A[0][0] > A[1][1] && A[0][0] > A[2][2] ) {
            float S = 2.0f * sqrtf( 1.0f + A[0][0] - A[1][1] - A[2][2]);
            Result.w = (A[2][1] - A[1][2] ) / S;
            Result.x = 0.25f * S;
            Result.y = (A[0][1] + A[1][0] ) / S;
            Result.z = (A[0][2] + A[2][0] ) / S;
        } else if (A[1][1] > A[2][2]) {
            float S = 2.0f * sqrtf( 1.0f + A[1][1] - A[0][0] - A[2][2]);
            Result.w = (A[0][2] - A[2][0] ) / S;
            Result.x = (A[0][1] + A[1][0] ) / S;
            Result.y = 0.25f * S;
            Result.z = (A[1][2] + A[2][1] ) / S;
        } else {
            float S = 2.0f * sqrtf( 1.0f + A[2][2] - A[0][0] - A[1][1] );
            Result.w = (A[1][0] - A[0][1] ) / S;
            Result.x = (A[0][2] + A[2][0] ) / S;
            Result.y = (A[1][2] + A[2][1] ) / S;
            Result.z = 0.25f * S;
        }
    }
    
    return(Result);
}

inline quat QuatFromM33(m33 Mat){
    quat Result = QuatFrom2DArray(Mat.E2);
    
    return(Result);
}

inline quat QuatLookAt(v3 Front, v3 Up){
    v3 Fwd = Normalize(Front);
    v3 Lft = Normalize(Cross(Up, Fwd));
    Up = Cross(Fwd, Lft);
    
    m33 Mat = MatrixFromRows(Lft, Up, Fwd);
    
    quat Result = QuatFromM33(Mat);
    
    return(Result);
}

/* Color math */
inline uint32_t PackRGBA(v4 Color){
    uint32_t Result = 
        (uint32_t)((Color.r * 255.0f + 0.5f)) |
        ((uint32_t)((Color.g * 255.0f) + 0.5f) << 8) |
        ((uint32_t)((Color.b * 255.0f) + 0.5f) << 16) |
        ((uint32_t)((Color.a * 255.0f) + 0.5f) << 24);
    
    return(Result);
}

inline v4 UnpackRGBA(uint32_t Color){
    v4 Result;
    
    Result.r = (float)(Color & 0xFF) * JOY_ONE_OVER_255;
    Result.g = (float)((Color >> 8) & 0xFF) * JOY_ONE_OVER_255;
    Result.b = (float)((Color >> 16) & 0xFF) * JOY_ONE_OVER_255;
    Result.a = (float)((Color >> 24) & 0xFF) * JOY_ONE_OVER_255;
    
    return(Result);
}

/* Rectangle math */
inline rc2 RcMinMax(v2 Min, v2 Max){
    rc2 Result;
    
    Result.Min = Min;
    Result.Max = Max;
    
    return(Result);
}

inline rc2 RcMinDim(v2 Min, v2 Dim){
    rc2 Result;
    
    Result.Min = Min;
    Result.Max = Min + Dim;
    
    return(Result);
}

inline v2 GetRectDim(rc2 A){
    v2 Result = V2(abs(A.Max.x - A.Min.x),
                   abs(A.Max.y - A.Min.y));
    
    return(Result);
}

inline float GetRectWidth(rc2 A){
    float Result = A.Max.x - A.Min.x;
    
    return(Result);
}

inline float GetRectHeight(rc2 A){
    float Result = A.Max.y - A.Min.y;
    
    return(Result);
}

inline v2 GetRectCenter(rc2 A){
    v2 Result = A.Min + GetRectDim(A) * 0.5f;
    
    return(Result);
}

inline v2 ClampInRect(v2 P, rc2 A){
    v2 Result;
    Result.x = Clamp(P.x, A.Min.x, A.Max.x);
    Result.y = Clamp(P.y, A.Min.y, A.Max.y);
    return(Result);
}

inline rc2 GetBoundingRect(rc2 A, rc2 B){
    rc2 Result;
    Result.Min.x = JOY_MATH_MIN(A.Min.x, B.Min.x);
    Result.Min.y = JOY_MATH_MIN(A.Min.y, B.Min.y);
    Result.Max.x = JOY_MATH_MAX(A.Max.x, B.Max.x);
    Result.Max.y = JOY_MATH_MAX(A.Max.y, B.Max.y);
    return(Result);
}

inline rc2 GrowRectByScale(rc2 A, v2 Scale){
    v2 RectDim = GetRectDim(A);
    v2 NewDim;
    NewDim.x = RectDim.x * Scale.x;
    NewDim.y = RectDim.y * Scale.y;
    v2 Center = A.Min + RectDim * 0.5f;
    rc2 Result;
    Result.Min = Center - NewDim * 0.5f;
    Result.Max = Result.Min + NewDim;
    return(Result);
}

inline rc2 GrowRectByScaledValue(rc2 A, v2 Value, float Scale){
    rc2 Result;
    
    v2 ValueScaled = Value * Scale;
    Result.Min.x = A.Min.x - ValueScaled.x;
    Result.Min.y = A.Min.y - ValueScaled.y;
    Result.Max.x = A.Max.x + ValueScaled.x;
    Result.Max.y = A.Max.y + ValueScaled.y;
    
    return(Result);
}

inline rc2 GrowRectByPixels(rc2 A, int PixelsCount){
    rc2 Result;
    Result.Min -= V2(PixelsCount, PixelsCount);
    Result.Max += V2(PixelsCount, PixelsCount);
    return(Result);
}

#define JOY_MATH_RCNORMSUBPX_VAL(v) v = (int)(v + 0.5f);
inline rc2 RectNormalizeSubpixel(rc2 A){
    JOY_MATH_RCNORMSUBPX_VAL(A.Min.x);
    JOY_MATH_RCNORMSUBPX_VAL(A.Min.y);
    JOY_MATH_RCNORMSUBPX_VAL(A.Max.x);
    JOY_MATH_RCNORMSUBPX_VAL(A.Max.y);
    
    return(A);
}

#endif
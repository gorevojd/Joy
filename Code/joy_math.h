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

struct Complex_Num {
	float re;
	float im;
};

struct Euler_Angles{
    float Yaw;
    float Pitch;
    float Roll;
};

//NOTE(dima): Structures
typedef union v2 {
	struct {
		float x;
		float y;
	};
    
	float e[2];
} v2;

typedef union v3 {
	struct {
		union{
            struct{
                float x;
                float y;
            };
            v2 xy;
        };
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
    
	float e[3];
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
    
	float e[4];
} v4;

typedef struct rc2{
    v2 min;
    v2 max;
} rc2;

typedef union m33 {
    float e[9];
    float e2[3][3];
    v3 Rows[3];
} m33;

typedef union m44 {
	float e[16];
    float e2[4][4];
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
inline float CopySign(float Val1, float Val2){
    float Result = abs(Val1) * (Val2 > 0.0f ? 1.0f : -1.0f);
    return(Result);
}

inline float Sqrt(float Value) {
	float res;
	res = sqrtf(Value);
	return(res);
}

inline float RSqrt(float Value) {
	float res;
	res = 1.0f / sqrtf(Value);
	return(res);
}

inline float Floor(float Value) {
	float res = floorf(Value);
	return(res);
}

inline float Ceil(float Value) {
	float res = ceilf(Value);
	return(res);
}

inline float Sin(float Rad) {
	float res = sinf(Rad);
	return(res);
}

inline float Cos(float Rad) {
	float res = cosf(Rad);
	return(res);
}

inline float Tan(float Rad) {
	float res = tanf(Rad);
	return(res);
}

inline float ASin(float Value) {
	float res = asinf(Value);
	return(res);
}

inline float ACos(float Value) {
	float res = acosf(Value);
	return(res);
}

inline float ATan(float Value) {
	float res = atan(Value);
	return(res);
}

inline float ATan2(float Y, float X) {
	float res = atan2f(Y, X);
	return(res);
}

inline float Exp(float Value) {
	float res = expf(Value);
	return(res);
}

inline float Log(float Value) {
	float res = logf(Value);
	return(res);
}

inline float Pow(float a, float b) {
	float res = powf(a, b);
	return(res);
}

inline float Lerp(float a, float b, float t) {
	float res = a + (b - a) * t;
    
	return(res);
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
	v2 res;
    
	res.x = Value;
	res.y = Value;
    
	return(res);
}

inline v2 V2(float x, float y) {
	v2 res;
    
	res.x = x;
	res.y = y;
    
	return(res);
}

inline v3 V3(v2 xy, float z) {
	v3 res;
    
	res.x = xy.x;
	res.y = xy.y;
	res.z = z;
    
	return(res);
}

inline v3 V3(float x, float y, float z) {
	v3 res;
    
	res.x = x;
	res.y = y;
	res.z = z;
    
	return(res);
}

inline v3 V3(float Value){
    v3 Res;
    
    Res.x = Value;
    Res.y = Value;
    Res.z = Value;
    
    return(Res);
}

inline v4 V4(float Value) {
	v4 res;
	res.x = Value;
	res.y = Value;
	res.z = Value;
	res.w = Value;
	return(res);
}

inline v4 V4(float x, float y, float z, float w) {
	v4 res;
    
	res.x = x;
	res.y = y;
	res.z = z;
	res.w = w;
    
	return(res);
}

inline v4 V4(v3 InitVector, float w) {
	v4 res;
	res.x = InitVector.x;
	res.y = InitVector.y;
	res.z = InitVector.z;
	res.w = w;
	return(res);
}

inline quat QuatI(){
	quat res;
    
	res.x = 0.0f;
	res.y = 0.0f;
	res.z = 0.0f;
	res.w = 1.0f;
    
	return(res);
}

inline quat Quat(float x, float y, float z, float w){
	quat res;
    
	res.x = x;
	res.y = y;
	res.z = z;
	res.w = w;
    
	return(res);
}

inline quat Quat(v3 Axis, float Angle){
	quat res;
    
    float HalfAngle = Angle * 0.5f;
    
	float S = Sin(HalfAngle);
	res.x = Axis.x * S;
	res.y = Axis.y * S;
	res.z = Axis.z * S;
	res.w = Cos(HalfAngle);
    
	return(res);
}

inline m33 M33I(){
    m33 res;
    
    res.Rows[0] = {1.0f, 0.0f, 0.0f};
    res.Rows[1] = {0.0f, 1.0f, 0.0f};
    res.Rows[2] = {0.0f, 0.0f, 1.0f};
    
    return(res);
}

inline m44 M44I(){
    m44 res;
    
    res.Rows[0] = { 1.0f, 0.0f, 0.0f, 0.0f };
    res.Rows[1] = { 0.0f, 1.0f, 0.0f, 0.0f };
    res.Rows[2] = { 0.0f, 0.0f, 1.0f, 0.0f };
    res.Rows[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    return(res);
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
    
    __m128 res = _mm_mul_ps(vX, M.mmRows[0]);
    res = _mm_add_ps(res, _mm_mul_ps(vY, M.mmRows[1]));
    res = _mm_add_ps(res, _mm_mul_ps(vZ, M.mmRows[2]));
    res = _mm_add_ps(res, _mm_mul_ps(vW, M.mmRows[3]));
    
    return(res);
}
#endif

inline m44 Mul(m44 A, m44 B) {
    m44 res = {};
    
#if JOY_ENABLE_SIMD_MATH
    res.mmRows[0] = MulVecMatSSE(A.mmRows[0], B);
    res.mmRows[1] = MulVecMatSSE(A.mmRows[1], B);
    res.mmRows[2] = MulVecMatSSE(A.mmRows[2], B);
    res.mmRows[3] = MulVecMatSSE(A.mmRows[3], B);
#else
    res.e[0] = A.e[0] * B.e[0] + A.e[1] * B.e[4] + A.e[2] * B.e[8] + A.e[3] * B.e[12];
    res.e[1] = A.e[0] * B.e[1] + A.e[1] * B.e[5] + A.e[2] * B.e[9] + A.e[3] * B.e[13];
    res.e[2] = A.e[0] * B.e[2] + A.e[1] * B.e[6] + A.e[2] * B.e[10] + A.e[3] * B.e[14];
    res.e[3] = A.e[0] * B.e[3] + A.e[1] * B.e[7] + A.e[2] * B.e[11] + A.e[3] * B.e[15];
    
    res.e[4] = A.e[4] * B.e[0] + A.e[5] * B.e[4] + A.e[6] * B.e[8] + A.e[7] * B.e[12];
    res.e[5] = A.e[4] * B.e[1] + A.e[5] * B.e[5] + A.e[6] * B.e[9] + A.e[7] * B.e[13];
    res.e[6] = A.e[4] * B.e[2] + A.e[5] * B.e[6] + A.e[6] * B.e[10] + A.e[7] * B.e[14];
    res.e[7] = A.e[4] * B.e[3] + A.e[5] * B.e[7] + A.e[6] * B.e[11] + A.e[7] * B.e[15];
    
    res.e[8] = A.e[8] * B.e[0] + A.e[9] * B.e[4] + A.e[10] * B.e[8] + A.e[11] * B.e[12];
    res.e[9] = A.e[8] * B.e[1] + A.e[9] * B.e[5] + A.e[10] * B.e[9] + A.e[11] * B.e[13];
    res.e[10] = A.e[8] * B.e[2] + A.e[9] * B.e[6] + A.e[10] * B.e[10] + A.e[11] * B.e[14];
    res.e[11] = A.e[8] * B.e[3] + A.e[9] * B.e[7] + A.e[10] * B.e[11] + A.e[11] * B.e[15];
    
    res.e[12] = A.e[12] * B.e[0] + A.e[13] * B.e[4] + A.e[14] * B.e[8] + A.e[15] * B.e[12];
    res.e[13] = A.e[12] * B.e[1] + A.e[13] * B.e[5] + A.e[14] * B.e[9] + A.e[15] * B.e[13];
    res.e[14] = A.e[12] * B.e[2] + A.e[13] * B.e[6] + A.e[14] * B.e[10] + A.e[15] * B.e[14];
    res.e[15] = A.e[12] * B.e[3] + A.e[13] * B.e[7] + A.e[14] * B.e[11] + A.e[15] * B.e[15];
#endif
    
    return(res);
}

inline v4 Mul(v4 V, m44 M) {
    v4 res;
    
    res.e[0] = V.e[0] * M.e[0] + V.e[1] * M.e[4] + V.e[2] * M.e[8] + V.e[3] * M.e[12];
    res.e[1] = V.e[0] * M.e[1] + V.e[1] * M.e[5] + V.e[2] * M.e[9] + V.e[3] * M.e[13];
    res.e[2] = V.e[0] * M.e[2] + V.e[1] * M.e[6] + V.e[2] * M.e[10] + V.e[3] * M.e[14];
    res.e[3] = V.e[0] * M.e[3] + V.e[1] * M.e[7] + V.e[2] * M.e[11] + V.e[3] * M.e[15];
    
    return(res);
}

inline void Mul(float Res[3], float A[3], float B[9]){
    Res[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
    Res[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
    Res[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];
}

inline v3 Mul(v3 A, m33 B){
    float TmpRes[3];
    v3 Res;
    Mul(Res.e, A.e, B.e);
    
    return(Res);
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

inline v3 operator*(v3 A, m33 B){
    v3 Res = Mul(A, B);
    
    return(Res);
}

inline v4 operator*(v4 A, m44 B){
    v4 res = Mul(A, B);
    
    return(res);
}

inline m44 operator*(m44 A, m44 B){
    m44 res = Mul(A, B);
    
    return(res);
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
    quat res = {};
    
    res.xyz = -Q.xyz;
    res.w = Q.w;
    
    return(res);
}

inline quat Inverse(quat Q){
    quat res = Conjugate(Q) / Dot(Q, Q);
    
    return(res);
}

inline quat Lerp(quat A, quat B, float t) {
    quat Result;
    
    float OneMinusT = 1.0f - t;
    
    Result.x = A.x * OneMinusT + B.x * t;
    Result.y = A.y * OneMinusT + B.y * t;
    Result.z = A.z * OneMinusT + B.z * t;
    Result.w = A.w * OneMinusT + B.w * t;
    
    Result = Normalize(Result);
    
    return(Result);
}

inline quat Slerp(quat A, quat B, float t){
    A = Normalize(A);
    B = Normalize(B);
    
    float CosTheta = Dot(A, B);
    float Theta = ACos(CosTheta);
    
    float OneOverSinTheta = 1.0f / Sin(Theta);
    float cA = OneOverSinTheta * Sin((1.0f - t) * Theta);
    float cB = OneOverSinTheta * Sin(t * Theta);
    
    quat Result;
    
    Result.x = cA * A.x + cB * B.x;
    Result.y = cA * A.y + cB * B.y;
    Result.z = cA * A.z + cB * B.z;
    Result.w = cA * A.w + cB * B.w;
    
    return(Result);
}

/*Matrix operations*/
inline m44 Identity(){
    m44 Result = {};
    
    Result.e[0] = 1.0f;
    Result.e[5] = 1.0f;
    Result.e[10] = 1.0f;
    Result.e[15] = 1.0f;
    
    return(Result);
}

inline m44 Floats2Matrix(float Array[16]){
    m44 Result = {};
    
    for(int i = 0; i < 16; i++){
        Result.e[i] = Array[i];
    }
    
    return(Result);
}

inline m44 TranslationMatrix(v3 Translation){
    m44 Result = Identity();
    
	Result.e[12] = Translation.x;
	Result.e[13] = Translation.y;
	Result.e[14] = Translation.z;
    
	return(Result);
}

inline m44 InverseTranslationMatrix(v3 Translation){
    m44 Result = Identity();
    
	Result.e[12] = -Translation.x;
	Result.e[13] = -Translation.y;
	Result.e[14] = -Translation.z;
    
    return(Result);
}

inline m44 ScalingMatrix(v3 Scale) {
    m44 Result = Identity();
    
    Result.e[0] = Scale.x;
    Result.e[5] = Scale.y;
    Result.e[10] = Scale.z;
    
    return(Result);
}

inline m44 ScalingMatrix(float Scale){
    m44 Result = Identity();
    
    Result.e[0] = Scale;
    Result.e[5] = Scale;
    Result.e[10] = Scale;
    
    return(Result);
}

inline m44 Transpose(m44 M) {
    m44 Result;
    
#if JOY_ENABLE_SIMD_MATH
    __m128 Tmp0, Tmp1, Tmp2, Tmp3;
    
    Tmp0 = _mm_shuffle_ps(M.mmRows[0], M.mmRows[1], 0x44);
    Tmp1 = _mm_shuffle_ps(M.mmRows[0], M.mmRows[1], 0xEE);
    Tmp2 = _mm_shuffle_ps(M.mmRows[2], M.mmRows[3], 0x44);
    Tmp3 = _mm_shuffle_ps(M.mmRows[2], M.mmRows[3], 0xEE);
    
    Result.mmRows[0] = _mm_shuffle_ps(Tmp0, Tmp2, 0x88);
    Result.mmRows[1] = _mm_shuffle_ps(Tmp0, Tmp2, 0xDD);
    Result.mmRows[2] = _mm_shuffle_ps(Tmp1, Tmp3, 0x88);
    Result.mmRows[3] = _mm_shuffle_ps(Tmp1, Tmp3, 0xDD);
#else
    for (int RowIndex = 0; RowIndex < 4; RowIndex++) {
        for (int ColumtIndex = 0; ColumtIndex < 4; ColumtIndex++) {
            Result.e[ColumtIndex * 4 + RowIndex] = M.e[RowIndex * 4 + ColumtIndex];
        }
    }
#endif
    
    return(Result);
}

inline m33 Transpose(m33 M){
    m33 Result = {};
    
    for (int RowIndex = 0; RowIndex < 3; RowIndex++) {
        for (int ColumtIndex = 0; ColumtIndex < 3; ColumtIndex++) {
            Result.e[ColumtIndex * 3 + RowIndex] = M.e[RowIndex * 3 + ColumtIndex];
        }
    }
    
    
    
    return(Result);
}

inline m44 InverseTransformMatrix(m44 Transformation){
    // NOTE(Dima): Extracting translation
    v4 Translation = Transformation.Rows[3];
    
    // NOTE(Dima): Setting last row to zero to leave only rotation and scaling
    Transformation.Rows[3] = V4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // NOTE(Dima): Inverting rotation and scale
    Transformation = Transpose(Transformation);
    
    m44 Result = InverseTranslationMatrix(Translation.xyz) * Transformation;
    
    return(Result);
}

inline m44 PerspectiveProjection(int Width, int Height, float Far, float Near)
{
    m44 Result = {};
    
    float AspectRatio = (float)Width / (float)Height;
    
    float S = 1.0f / (Tan(45.0f * 0.5f * JOY_DEG2RAD));
    float A = S / AspectRatio;
    float B = S;
    float OneOverFarMinusNear = 1.0f / (Far - Near);
    Result.e[0] = A;
    Result.e[5] = B;
    Result.e[10] = -(Far + Near) * OneOverFarMinusNear;
    Result.e[14] = -(2.0f * Far * Near) * OneOverFarMinusNear;
    Result.e[11] = -1.0f;
    
    return(Result);
}

inline m44 PerspectiveProjectionScreen(int Width, int Height, float Far, float Near){
    m44 Result = {};
    
    float MinusOneOverFarMinusNear = -1.0f / (Far - Near);
    Result.e[0] = 2.0f * Near / (float)Width;
    Result.e[5] = 2.0f * Near / (float)Height;
    Result.e[8] = 1.0f;
    Result.e[9] = 1.0f;
    Result.e[10] = (Far + Near) * MinusOneOverFarMinusNear;
    Result.e[11] = -1.0f;
    Result.e[12] = 1.0f;
    Result.e[13] = 1.0f;
    Result.e[14] = (2.0f * Far * Near) * MinusOneOverFarMinusNear;
    
    return(Result);
}

inline m44 OrthographicProjection(int Width, int Height) {
    m44 Result = {};
    
    Result.e[0] = 2.0f / (float)Width;
    Result.e[12] = -1.0f;
    Result.e[5] = 2.0f / (float)Height;
    Result.e[13] = -1.0f;
    Result.e[10] = 1.0f;
    Result.e[15] = 1.0f;
    
    return(Result);
}

inline m44 LookAt(v3 Pos, v3 TargetPos, v3 WorldUp) {
    m44 Result;
    
    v3 Fwd = TargetPos - Pos;
    Fwd = NOZ(Fwd);
    
    v3 Left = Normalize(Cross(WorldUp, Fwd));
    v3 Up = Normalize(Cross(Fwd, Left));
    
    v3 Eye = Pos;
    
    Result.e[0] = Left.x;
    Result.e[1] = Up.x;
    Result.e[2] = Fwd.x;
    Result.e[3] = 0.0f;
    
    Result.e[4] = Left.y;
    Result.e[5] = Up.y;
    Result.e[6] = Fwd.y;
    Result.e[7] = 0.0f;
    
    Result.e[8] = Left.z;
    Result.e[9] = Up.z;
    Result.e[10] = Fwd.z;
    Result.e[11] = 0.0f;
    
    Result.e[12] = -Dot(Left, Eye);
    Result.e[13] = -Dot(Up, Eye);
    Result.e[14] = -Dot(Fwd, Eye);
    Result.e[15] = 1.0f;
    
    return(Result);
}

/*Conversions*/
inline m44 MatrixFromRows(v4 R1, v4 R2, v4 R3, v4 R4){
    m44 res = {};
    
    res.Rows[0] = R1;
    res.Rows[1] = R2;
    res.Rows[2] = R3;
    res.Rows[3] = R4;
    
    return(res);
}

inline m33 MatrixFromRows(v3 R1, v3 R2, v3 R3){
    m33 res = {};
    
    res.Rows[0] = R1;
    res.Rows[1] = R2;
    res.Rows[2] = R3;
    
    return(res);
}

inline m33 Quat2M33(quat Q){
    m33 res = {};
    
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    res.e[0] = 1.0f - 2.0f * (y2 + z2);
    res.e[1] = 2.0f * (xy + zw);
    res.e[2] = 2.0f * (xz - yw);
    
    res.e[3] = 2.0f * (xy - zw);
    res.e[4] = 1.0f - 2.0f * (x2 + z2);
    res.e[5] = 2.0f * (yz + xw);
    
    res.e[6] = 2.0f * (xz + yw);
    res.e[7] = 2.0f * (yz - xw);
    res.e[8] = 1.0f - 2.0f * (x2 + y2);
    
    return(res);
}

inline m44 Quat2M44(quat Q){
    m44 res = {};
    
    float xx = Q.x * Q.x;
    float yy = Q.y * Q.y;
    float zz = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    res.e[0] = 1.0f - 2.0f * (yy + zz);
    res.e[1] = 2.0f * (xy + zw);
    res.e[2] = 2.0f * (xz - yw);
    res.e[3] = 0.0f;
    
    res.e[4] = 2.0f * (xy - zw);
    res.e[5] = 1.0f - 2.0f * (xx + zz);
    res.e[6] = 2.0f * (yz + xw);
    res.e[7] = 0.0f;
    
    res.e[8] = 2.0f * (xz + yw);
    res.e[9] = 2.0f * (yz - xw);
    res.e[10] = 1.0f - 2.0f * (xx + yy);
    res.e[11] = 0.0f;
    
    res.e[12] = 0.0f;
    res.e[13] = 0.0f;
    res.e[14] = 0.0f;
    res.e[15] = 1.0f;
    
    return(res);
}

inline m44 RotationMatrix(quat Q){
    return(Quat2M44(Q));
}


#if 0
inline v3 GetQuatLeft(quat Q){
    
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    v3 Result;
    Result.x = 1.0f - 2.0f * (y2 + z2);
    Result.y = 2.0f * (xy + zw);
    Result.z = 2.0f * (xz - yw);
    
    return(Result);
}

inline v3 GetQuatUp(quat Q){
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    v3 Result;
    
    Result.x = 2.0f * (xy - zw);
    Result.y = 1.0f - 2.0f * (x2 + z2);
    Result.z = 2.0f * (yz + xw);
    
    return(Result);
}

inline v3 GetQuatFront(quat Q){
    float x2 = Q.x * Q.x;
    float y2 = Q.y * Q.y;
    float z2 = Q.z * Q.z;
    
    float xy = Q.x * Q.y;
    float zw = Q.z * Q.w;
    float xz = Q.x * Q.z;
    float yw = Q.y * Q.w;
    float yz = Q.y * Q.z;
    float xw = Q.x * Q.w;
    
    v3 Result;
    
    Result.x = 2.0f * (xz + yw);
    Result.y = 2.0f * (yz - xw);
    Result.z = 1.0f - 2.0f * (x2 + y2);
    
    return(Result);
}
#endif

inline quat QuatFrom2DArray(float A[3][3]){
    quat res;
    
    float Trace = A[0][0] + A[1][1] + A[2][2]; // I removed + 1.0f; see discussion with Ethan
    if( Trace > 0 ) {// I changed M_EPSILON to 0
        float S = 0.5f / sqrtf(Trace + 1.0f);
        res.w = 0.25f / S;
        res.x = ( A[1][2] - A[2][1] ) * S;
        res.y = ( A[2][0] - A[0][2] ) * S;
        res.z = ( A[0][1] - A[1][0] ) * S;
    } else {
        if ( A[0][0] > A[1][1] && A[0][0] > A[2][2] ) {
            float S = 2.0f * sqrtf( 1.0f + A[0][0] - A[1][1] - A[2][2]);
            res.w = (A[1][2] - A[2][1] ) / S;
            res.x = 0.25f * S;
            res.y = (A[1][0] + A[0][1] ) / S;
            res.z = (A[2][0] + A[0][2] ) / S;
        } else if (A[1][1] > A[2][2]) {
            float S = 2.0f * sqrtf( 1.0f + A[1][1] - A[0][0] - A[2][2]);
            res.w = (A[2][0] - A[0][2] ) / S;
            res.x = (A[1][0] + A[0][1] ) / S;
            res.y = 0.25f * S;
            res.z = (A[2][1] + A[1][2] ) / S;
        } else {
            float S = 2.0f * sqrtf( 1.0f + A[2][2] - A[0][0] - A[1][1] );
            res.w = (A[0][1] - A[1][0] ) / S;
            res.x = (A[2][0] + A[0][2] ) / S;
            res.y = (A[2][1] + A[1][2] ) / S;
            res.z = 0.25f * S;
        }
    }
    
    return(res);
}

inline quat QuatFromM33(m33 Mat){
    quat res = QuatFrom2DArray(Mat.e2);
    
    return(res);
}

inline quat QuatLookAt(v3 Front, v3 Up){
    v3 Fwd = Normalize(Front);
    v3 Lft = Normalize(Cross(Up, Fwd));
    Up = Cross(Fwd, Lft);
    
    m33 Mat = MatrixFromRows(Lft, Up, Fwd);
    
    quat res = QuatFromM33(Mat);
    
    return(res);
}

inline Euler_Angles Quat2Euler(quat q){
    Euler_Angles Result = {};
    
    // roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    Result.Roll = ATan2(sinr_cosp, cosr_cosp);
    
    // pitch (y-axis rotation)
    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (abs(sinp) >= 1.0f)
        Result.Pitch = CopySign(JOY_PI / 2.0f, sinp); // use 90 degrees if out of range
    else
        Result.Pitch = ASin(sinp);
    
    // yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    Result.Yaw = ATan2(siny_cosp, cosy_cosp);
    
    return(Result);
}

inline quat Euler2Quat(Euler_Angles A){
    quat Result = {};
    
    // Abbreviations for the various angular functions
    float cy = cosf(A.Yaw * 0.5f);
    float sy = sinf(A.Yaw * 0.5f);
    float cp = cosf(A.Pitch * 0.5f);
    float sp = sinf(A.Pitch * 0.5f);
    float cr = cosf(A.Roll * 0.5f);
    float sr = sinf(A.Roll * 0.5f);
    
    Result.w = cy * cp * cr + sy * sp * sr;
    Result.x = cy * cp * sr - sy * sp * cr;
    Result.y = sy * cp * sr + cy * sp * cr;
    Result.z = sy * cp * cr - cy * sp * sr;
    
    return(Result);
}

/* Color math */
inline uint32_t PackRGBA(v4 Color){
    uint32_t res = 
        (uint32_t)((Color.r * 255.0f + 0.5f)) |
        ((uint32_t)((Color.g * 255.0f) + 0.5f) << 8) |
        ((uint32_t)((Color.b * 255.0f) + 0.5f) << 16) |
        ((uint32_t)((Color.a * 255.0f) + 0.5f) << 24);
    
    return(res);
}

inline v4 UnpackRGBA(uint32_t Color){
    v4 res;
    
    res.r = (float)(Color & 0xFF) * JOY_ONE_OVER_255;
    res.g = (float)((Color >> 8) & 0xFF) * JOY_ONE_OVER_255;
    res.b = (float)((Color >> 16) & 0xFF) * JOY_ONE_OVER_255;
    res.a = (float)((Color >> 24) & 0xFF) * JOY_ONE_OVER_255;
    
    return(res);
}

inline uint32_t PackRGB_R10G12B10(v3 Color){
    uint32_t Result = 
        ((uint32_t)(Color.r * 1023.0f + 0.5f)) |
        ((uint32_t)((Color.g * 4095.0f + 0.5f)) << 10) |
        ((uint32_t)((Color.b * 1023.0f + 0.5f)) << 22);
    
    return(Result);
}

inline v3 UnpackRGB_R10G12B10(uint32_t Color){
    v3 Result;
    
    Result.r = (float)((Color & 1023) / 1023.0f);
    Result.g = (float)(((Color >> 10) & 4095) / 4095.0f);
    Result.b = (float)(((Color >> 22) & 1023) / 1023.0f);
    
    return(Result);
}

/*Plane math*/
inline v4 NormalizePlane(v4 Plane) {
    float NormalLen = Magnitude(Plane.rgb);
    
    v4 Result;
    Result.A = Plane.A / NormalLen;
    Result.B = Plane.B / NormalLen;
    Result.C = Plane.C / NormalLen;
    Result.D = Plane.D / NormalLen;
    
    return(Result);
}

inline float PlanePointTest(v4 Plane, v3 Point) {
    float Res = Dot(Plane.ABC, Point) + Plane.D;
    
    return(Res);
}

/* Complex number math */
inline Complex_Num operator+(Complex_Num a, Complex_Num b) {
    Complex_Num res;
    
    res.re = a.re + b.re;
    res.im = a.im + b.im;
    
    return(res);
}

inline Complex_Num operator-(Complex_Num a, Complex_Num b) {
    Complex_Num res;
    
    res.re = a.re - b.re;
    res.im = a.im - b.im;
    
    return(res);
}

inline Complex_Num operator*(Complex_Num a, Complex_Num b) {
    Complex_Num res;
    
    res.re = a.re * b.re - a.im * b.im;
    res.im = a.im * b.re + a.re * b.im;
    
    return(res);
}

inline Complex_Num operator*(Complex_Num a, float s) {
    Complex_Num res;
    
    res.re = a.re * s;
    res.im = a.im * s;
    
    return(res);
}

/* Rectangle math */
inline rc2 RcMinMax(v2 Min, v2 Max){
    rc2 res;
    
    res.min = Min;
    res.max = Max;
    
    return(res);
}

inline rc2 RcMinDim(v2 Min, v2 Dim){
    rc2 res;
    
    res.min = Min;
    res.max = Min + Dim;
    
    return(res);
}

inline v2 GetRectDim(rc2 A){
    v2 res = V2(abs(A.max.x - A.min.x),
                abs(A.max.y - A.min.y));
    
    return(res);
}

inline float GetRectWidth(rc2 A){
    float res = A.max.x - A.min.x;
    
    return(res);
}

inline float GetRectHeight(rc2 A){
    float res = A.max.y - A.min.y;
    
    return(res);
}

inline v2 GetRectCenter(rc2 A){
    v2 res = A.min + GetRectDim(A) * 0.5f;
    
    return(res);
}

inline v2 ClampInRect(v2 P, rc2 A){
    v2 res;
    res.x = Clamp(P.x, A.min.x, A.max.x);
    res.y = Clamp(P.y, A.min.y, A.max.y);
    return(res);
}

inline rc2 GetBoundingRect(rc2 A, rc2 B){
    rc2 res;
    res.min.x = JOY_MATH_MIN(A.min.x, B.min.x);
    res.min.y = JOY_MATH_MIN(A.min.y, B.min.y);
    res.max.x = JOY_MATH_MAX(A.max.x, B.max.x);
    res.max.y = JOY_MATH_MAX(A.max.y, B.max.y);
    return(res);
}

inline rc2 GrowRectByScale(rc2 A, v2 Scale){
    v2 RectDim = GetRectDim(A);
    v2 NewDim;
    NewDim.x = RectDim.x * Scale.x;
    NewDim.y = RectDim.y * Scale.y;
    v2 Center = A.min + RectDim * 0.5f;
    rc2 res;
    res.min = Center - NewDim * 0.5f;
    res.max = res.min + NewDim;
    return(res);
}

inline rc2 GrowRectByScaledValue(rc2 A, v2 Value, float Scale){
    rc2 res;
    
    v2 ValueScaled = Value * Scale;
    res.min.x = A.min.x - ValueScaled.x;
    res.min.y = A.min.y - ValueScaled.y;
    res.max.x = A.max.x + ValueScaled.x;
    res.max.y = A.max.y + ValueScaled.y;
    
    return(res);
}

inline rc2 GrowRectByPixels(rc2 A, int PixelsCount){
    rc2 res = A;
    res.min -= V2(PixelsCount, PixelsCount);
    res.max += V2(PixelsCount, PixelsCount);
    return(res);
}

#define JOY_MATH_RCNORMSUBPX_VAL(v) v = (int)(v + 0.5f);
inline rc2 RectNormalizeSubpixel(rc2 A){
    JOY_MATH_RCNORMSUBPX_VAL(A.min.x);
    JOY_MATH_RCNORMSUBPX_VAL(A.min.y);
    JOY_MATH_RCNORMSUBPX_VAL(A.max.x);
    JOY_MATH_RCNORMSUBPX_VAL(A.max.y);
    
    return(A);
}

inline int BoxIntersectsWithBox(rc2 Box1, rc2 Box2) {
    v2 Box1Dim = GetRectDim(Box1);
    v2 Box2Dim = GetRectDim(Box2);
    
    float DistBetweenCentersX = abs((Box1.min.x - Box2.min.x) * 2.0f + (Box1Dim.x - Box2Dim.x));
    float DistBetweenCentersY = abs((Box1.min.y - Box2.min.y) * 2.0f + (Box1Dim.y - Box2Dim.y));
    
    int IntersectionHappens =
        ((DistBetweenCentersX < Box1Dim.x + Box2Dim.x) &&
         (DistBetweenCentersY < Box1Dim.y + Box2Dim.y));
    
    return(IntersectionHappens);
}

// NOTE(Dima): Other math
inline float CalcScreenFitHeight(
float bmpW, float bmpH,
float scrW, float scrH)
{
    float Result = scrH;
    
    float ar1 = bmpW / bmpH;
    float ar2 = scrW / scrH;
    
    float bmpWoverH = bmpW / bmpH;
    
    
    if(ar1 > ar2){
        Result = scrH;
    }
    else{
        float c1 = scrH / bmpH;
        float scaledH = bmpH * c1;
        float scaledW = scaledH * bmpWoverH;
        Result = scaledH * scrW / scaledW;
    }
    
    return(Result);
}

inline rc2 BottomLeftToTopLeftRectange(rc2 Rect, float ScreenHeight)
{
    rc2 Result = {};
    
    Result.min.x = Rect.min.x;
    Result.max.x = Rect.max.x;
    Result.min.y = ScreenHeight - Rect.max.y;
    Result.max.y = ScreenHeight - Rect.min.y;
    
    return(Result);
}

inline float BiunitToUnit(float x){
    float Result = x * 0.5f + 0.5f;
    
    return(Result);
}

inline float UnitToBiunit(float x){
    float Result = x * 2.0f - 1.0f;
    
    return(Result);
}

inline v2 UnitToBiunit(v2 V){
    v2 Result;
    
    Result.x = UnitToBiunit(V.x);
    Result.y = UnitToBiunit(V.y);
    
    return(Result);
}

inline v3 UnitToBiunit(v3 V){
    v3 Result;
    
    Result.x = UnitToBiunit(V.x);
    Result.y = UnitToBiunit(V.y);
    Result.z = UnitToBiunit(V.z);
    
    return(Result);
}

inline v2 BiunitToUnit(v2 V){
    v2 Result;
    
    Result.x = BiunitToUnit(V.x);
    Result.y = BiunitToUnit(V.y);
    
    return(Result);
}

inline v3 BiunitToUnit(v3 V){
    v3 Result;
    
    Result.x = BiunitToUnit(V.x);
    Result.y = BiunitToUnit(V.y);
    Result.z = BiunitToUnit(V.z);
    
    return(Result);
}

inline float SignNotZero(float Value){
    float Result = (Value >= 0.0f) ? 1.0f : -1.0f;
    
    return(Result);
}

inline v2 SignNotZero(v2 Value){
    v2 Result;
    
    Result.x = SignNotZero(Value.x);
    Result.y = SignNotZero(Value.y);
    
    return(Result);
}


inline v2 AbsVector(v2 Vector){
    v2 AbsVector = V2(
        abs(Vector.x),
        abs(Vector.y));
    
    return(AbsVector);
}

inline v3 AbsVector(v3 Vector){
    v3 AbsVector = V3(
        abs(Vector.x),
        abs(Vector.y),
        abs(Vector.z));
    
    return(AbsVector);
}

inline v2 OctToBiunit(v3 Norm){
    v3 Absol = AbsVector(Norm);
    
    float NormLen = Absol.x + Absol.y + Absol.z;
    float InvNorm = 1.0f / NormLen;
    
    v2 Result = Norm.xy * InvNorm;
    
    if(Norm.z < 0.0f){
        Result = Hadamard(V2(1.0f, 1.0f) - AbsVector(V2(Norm.y, Norm.x)), SignNotZero(Result));
    }
    
    return(Result);
}

inline v3 BiunitToOct(v2 Coord){
    v2 Absol = AbsVector(Coord);
    v3 Res = V3(Coord.x, Coord.y, 1.0f - Absol.x - Absol.y);
    if(Res.z < 0.0f){
        Res.xy = Hadamard(V2(1.0f, 1.0f) - AbsVector(V2(Res.y, Res.x)), SignNotZero(Res.xy));
    }
    
    return(NOZ(Res));
}

#endif
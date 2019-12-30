#include "joy_software_renderer.h"
#include "joy_render_stack.h"
#include "joy_simd.h"
#include "joy_platform.h"


#include <intrin.h>


#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"


void RenderOneBitmapIntoAnother(
Bmp_Info* to, 
Bmp_Info* what,
int startX,
int startY,
v4 modulationColor) 
{
	float OneOver255 = 1.0f / 255.0f;
    
	int MaxToX = startX + what->width;
	int MaxToY = startY + what->height;
    
	Assert(MaxToX <= to->width);
	Assert(MaxToY <= to->height);
    
	u32 SrcX = 0;
	u32 SrcY = 0;
    
	for (int Y = startY; Y < MaxToY; Y++) {
		SrcY = Y - startY;
        
		for (int X = startX; X < MaxToX; X++) {
			SrcX = X - startX;
            
			u32* Out = (u32*)to->pixels + Y * to->width + X;
			v4 DstInitColor = UnpackRGBA(*Out);
            
            
			u32* From = (u32*)what->pixels + SrcY * what->width + SrcX;
            
			v4 FromColor = UnpackRGBA(*From);
            
			v4 ResultColor = FromColor * modulationColor;
            
			//NOTE(dima): Calculating blend alpha value
			float BlendAlpha = ResultColor.a;
            
			ResultColor.x = ResultColor.x + DstInitColor.x * (1.0f - BlendAlpha);
			ResultColor.y = ResultColor.y + DstInitColor.y * (1.0f - BlendAlpha);
			ResultColor.z = ResultColor.z + DstInitColor.z * (1.0f - BlendAlpha);
			ResultColor.a = ResultColor.a + DstInitColor.a - ResultColor.a * DstInitColor.a;
            
			u32 ColorValue = PackRGBA(ResultColor);
			*Out = ColorValue;
            
			SrcX++;
		}
        
		SrcY++;
	}
}


void RenderRGBA2BGRA(Bmp_Info* buf, rc2 clipRect){
	int MinX = 0;
	int MaxX = buf->width;
	int MinY = 0;
	int MaxY = buf->height;
    
    MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)buf->pixels + DestY * buf->width + DestX;
            
            v4 LoadColor = UnpackRGBA(*OutDest);
            v4 StoreColor = V4(
                LoadColor.b,
                LoadColor.g,
                LoadColor.r,
                LoadColor.a);
            
            u32 OutColor = PackRGBA(StoreColor);
            
            *OutDest = OutColor;
		}
	}
}


void RenderRGBA2BGRASSE(Bmp_Info* buf, rc2 clipRect) {
	
	int MinX = 0;
	int MaxX = buf->width;
	int MinY = 0;
	int MaxY = buf->height;
    
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX += 4) {
			__m128i mmHorzIndex = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
            
            void* OutDest = (u8*)buf->pixels + DestY * buf->pitch + DestX * 4;
            
			__m128i mmPreDestColor = _mm_loadu_si128((__m128i*)OutDest);
            
			__m128 mmPreDestColor_r = MM_UNPACK_COLOR_CHANNEL0(mmPreDestColor);
			__m128 mmPreDestColor_g = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 8);
			__m128 mmPreDestColor_b = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 16);
			__m128 mmPreDestColor_a = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 24);
            
            __m128 mmResultColorTemp = mmPreDestColor_r;
            mmPreDestColor_r = mmPreDestColor_b;
            mmPreDestColor_b = mmResultColorTemp;
            
            __m128i mmOutColorSh_r = _mm_cvtps_epi32(_mm_mul_ps(mmPreDestColor_r, mm255));
            __m128i mmOutColorSh_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmPreDestColor_g, mm255)), 8);
            __m128i mmOutColorSh_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmPreDestColor_b, mm255)), 16);
            __m128i mmOutColorSh_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmPreDestColor_a, mm255)), 24);
            
            __m128i mmOutColor = _mm_or_si128(
                _mm_or_si128(mmOutColorSh_r, mmOutColorSh_g),
                _mm_or_si128(mmOutColorSh_b, mmOutColorSh_a));
            
            __m128i mmResultColor = mmOutColor;
            
#if 0
            /*Mask with end of screen mask*/
            //__m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmDestX), mmDestWidth);
            __m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmHorzIndex), mmMaxXF);
            
			__m128i mmResultColor = _mm_castps_si128(_mm_or_ps(
				_mm_and_ps(_mm_castsi128_ps(mmOutColor), mmEndScreenMask),
				_mm_andnot_ps(mmEndScreenMask, _mm_castsi128_ps(mmPreDestColor))));
#endif
            _mm_storeu_si128((__m128i*)OutDest, mmResultColor);
		}
	}
}


void RenderClear(Bmp_Info* buf, v3 Color, rc2 clipRect) {
	v4 ResColor = V4(Color.x, Color.y, Color.z, 1.0f);
	u32 OutColor = PackRGBA(ResColor);
    
	int MinX = 0;
	int MaxX = buf->width;
	int MinY = 0;
	int MaxY = buf->height;
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)buf->pixels + DestY * buf->width + DestX;
            
			*OutDest = OutColor;
		}
	}
}


void RenderClearSSE(Bmp_Info* buf, v3 Color, rc2 clipRect) {
	v4 ResColor = V4(Color.x, Color.y, Color.z, 1.0f);
	u32 OutColor = PackRGBA(ResColor);
    
	int MinX = 0;
	int MaxX = buf->width;
	int MinY = 0;
	int MaxY = buf->height;
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
    
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    
	__m128i mmOutColor;
	__m128 mmOutColor_r = _mm_set1_ps(Color.r);
	__m128 mmOutColor_g = _mm_set1_ps(Color.g);
	__m128 mmOutColor_b = _mm_set1_ps(Color.b);
	__m128 mmOutColor_a = _mm_set1_ps(1.0f);
    
	__m128i mmOutColorSh_r = _mm_cvtps_epi32(_mm_mul_ps(mmOutColor_r, mm255));
    __m128i mmOutColorSh_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_g, mm255)), 8);
	__m128i mmOutColorSh_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_b, mm255)), 16);
	__m128i mmOutColorSh_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_a, mm255)), 24);
	
	mmOutColor = _mm_or_si128(
		_mm_or_si128(mmOutColorSh_r, mmOutColorSh_g),
		_mm_or_si128(mmOutColorSh_b, mmOutColorSh_a));
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX += 4) {
			//u32* OutDest = (u32*)buf->pixels + DestY * buf->width + DestX;
            
#if 1
			__m128i mmHorzIndex = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
            
			u32* OutDest = (u32*)((u8*)buf->pixels + DestY * buf->pitch + DestX * 4);
            
            __m128i mmPreDestColor = _mm_loadu_si128((__m128i*)OutDest);
            
            /*Mask with end of screen mask*/
            //__m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmDestX), mmDestWidth);
            __m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmHorzIndex), mmMaxXF);
            
			__m128i mmResultColor = _mm_castps_si128(_mm_or_ps(
				_mm_and_ps(_mm_castsi128_ps(mmOutColor), mmEndScreenMask),
				_mm_andnot_ps(mmEndScreenMask, _mm_castsi128_ps(mmPreDestColor))));
#endif
            
            
			_mm_storeu_si128((__m128i*)OutDest, mmResultColor);
		}
	}
}

void RenderGradientHorz(Bmp_Info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect){
	
    v2 P = rect.min;
    v2 Dim = GetRectDim(rect);
    
    int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
    int SaveMinX = MinX;
    int SaveMinY = MinY;
    
    int SaveDimX = MaxX - MinX;
    int SaveDimY = MaxY - MinY;
    
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	float OneOverWidth = 1.0f / (float)SaveDimX;
	float OneOverHeight = 1.0f / (float)SaveDimY;
    
	for (int VertIndex = MinY; VertIndex < MaxY; VertIndex++) {
        
		float DeltaV = (float)(VertIndex - SaveMinY) * OneOverHeight;
        
		u32* Pixel = (u32*)((u8*)buf->pixels + VertIndex * buf->pitch + MinX * 4);
		for (int HorzIndex = MinX; HorzIndex < MaxX; HorzIndex++) {
			float DeltaU = (float)(HorzIndex - SaveMinX) * OneOverWidth;
            
			v4 OutColor;
			OutColor.x = color1.x + (Color2.x - color1.x) * DeltaU;
			OutColor.y = color1.y + (Color2.y - color1.y) * DeltaU;
			OutColor.z = color1.z + (Color2.z - color1.z) * DeltaU;
			OutColor.w = 1.0f;
            
			u32 ColorByteRep = PackRGBA(OutColor);
            
			*Pixel++ = ColorByteRep;
		}
	}
}

void RenderGradientHorzSSE(Bmp_Info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect) {
    v2 P = rect.min;
    v2 Dim = GetRectDim(rect);
    
    int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
    int SaveMinX = MinX;
    int SaveMinY = MinY;
    
    int SaveDimX = MaxX - MinX;
    int SaveDimY = MaxY - MinY;
	
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
    __m128i mmMinX = _mm_set1_epi32(MinX);
    __m128i mmMinY = _mm_set1_epi32(MinY);
    __m128i mmMaxX = _mm_set1_epi32(MaxX);
	
    __m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    __m128 mmMinXF = _mm_cvtepi32_ps(mmMinX);
    __m128 mmMinYF = _mm_cvtepi32_ps(mmMinY);
    
	__m128 mmOne = _mm_set1_ps(1.0f);
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    
	__m128 mmColor1_r = _mm_set1_ps(color1.r);
	__m128 mmColor1_g = _mm_set1_ps(color1.g);
	__m128 mmColor1_b = _mm_set1_ps(color1.b);
    
    __m128 mmColorDiff_r = _mm_set1_ps(Color2.r - color1.r);
    __m128 mmColorDiff_g = _mm_set1_ps(Color2.g - color1.g);
    __m128 mmColorDiff_b = _mm_set1_ps(Color2.b - color1.b);
    
	float OneOverWidth = 1.0f / (float)SaveDimX;
	float OneOverHeight = 1.0f / (float)SaveDimY;
    
	__m128 mmOneOverWidth = _mm_set1_ps(OneOverWidth);
	__m128 mmOneOverHeight = _mm_set1_ps(OneOverHeight);
    
	__m128i mmFF = _mm_set1_epi32(0xFF);
    
	for (int VertIndex = MinY; VertIndex < MaxY; VertIndex++) {
        
		float DeltaV = (float)(VertIndex - SaveMinY) * OneOverHeight;
        
		for (int HorzIndex = MinX; HorzIndex < MaxX; HorzIndex += 4) {
			
			u32* Pixel = (u32*)((u8*)buf->pixels + VertIndex * buf->width * 4 + HorzIndex * 4);
			
            int TmpIndex = HorzIndex - SaveMinX;
			__m128i mmHorzIndex = _mm_setr_epi32(TmpIndex, TmpIndex + 1, TmpIndex + 2, TmpIndex + 3);
			__m128 mmDeltaU = _mm_mul_ps(_mm_cvtepi32_ps(mmHorzIndex), mmOneOverWidth);
            
			__m128 mmOutColor_r = _mm_add_ps(_mm_mul_ps(mmDeltaU, mmColorDiff_r), mmColor1_r);
			__m128 mmOutColor_g = _mm_add_ps(_mm_mul_ps(mmDeltaU, mmColorDiff_g), mmColor1_g);
			__m128 mmOutColor_b = _mm_add_ps(_mm_mul_ps(mmDeltaU, mmColorDiff_b), mmColor1_b);
			__m128 mmOutColor_a = mmOne;
            
			__m128i mmOutColorShifted_r = _mm_cvtps_epi32(_mm_mul_ps(mmOutColor_r, mm255));
			__m128i mmOutColorShifted_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_g, mm255)), 8);
			__m128i mmOutColorShifted_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_b, mm255)), 16);
			__m128i mmOutColorShifted_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_a, mm255)), 24);
            
			__m128i mmOutColor = _mm_or_si128(
				_mm_or_si128(
                mmOutColorShifted_a, 
                mmOutColorShifted_b),
				_mm_or_si128(
                mmOutColorShifted_g, 
                mmOutColorShifted_r));
            
			_mm_storeu_si128((__m128i*)Pixel, mmOutColor);
		}
	}
}


void RenderGradientVert(Bmp_Info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect){
	
    v2 P = rect.min;
    v2 Dim = GetRectDim(rect);
    
    int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
    int SaveMinX = MinX;
    int SaveMinY = MinY;
    
    int SaveDimX = MaxX - MinX;
    int SaveDimY = MaxY - MinY;
    
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
    
	float OneOverWidth = 1.0f / (float)SaveDimX;
	float OneOverHeight = 1.0f / (float)SaveDimY;
    
	for (int VertIndex = MinY; VertIndex < MaxY; VertIndex++) {
        u32* Pixel = (u32*)((u8*)buf->pixels + VertIndex * buf->pitch + MinX * 4);
		
		float DeltaV = (float)(VertIndex - SaveMinY) * OneOverHeight;
        
        v4 OutColor;
        OutColor.x = color1.x + (Color2.x - color1.x) * DeltaV;
        OutColor.y = color1.y + (Color2.y - color1.y) * DeltaV;
        OutColor.z = color1.z + (Color2.z - color1.z) * DeltaV;
        OutColor.w = 1.0f;
        
        u32 ColorByteRep = PackRGBA(OutColor);
        
        for (int HorzIndex = MinX; HorzIndex < MaxX; HorzIndex++) {
			*Pixel++ = ColorByteRep;
		}
	}
}

void RenderGradientVertSSE(Bmp_Info* buf, rc2 rect, v3 color1, v3 color2, rc2 clipRect) {
    v2 P = rect.min;
    v2 Dim = GetRectDim(rect);
    
    int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
    int SaveMinX = MinX;
    int SaveMinY = MinY;
    
    int SaveDimX = MaxX - MinX;
    int SaveDimY = MaxY - MinY;
	
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
    __m128i mmMinX = _mm_set1_epi32(MinX);
    __m128i mmMinY = _mm_set1_epi32(MinY);
    __m128i mmMaxX = _mm_set1_epi32(MaxX);
	
    __m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    __m128 mmMinXF = _mm_cvtepi32_ps(mmMinX);
    __m128 mmMinYF = _mm_cvtepi32_ps(mmMinY);
    
	__m128 mmOne = _mm_set1_ps(1.0f);
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    
	__m128 mmColor1_r = _mm_set1_ps(color1.r);
	__m128 mmColor1_g = _mm_set1_ps(color1.g);
	__m128 mmColor1_b = _mm_set1_ps(color1.b);
    
    __m128 mmColorDiff_r = _mm_set1_ps(color2.r - color1.r);
    __m128 mmColorDiff_g = _mm_set1_ps(color2.g - color1.g);
    __m128 mmColorDiff_b = _mm_set1_ps(color2.b - color1.b);
    
	float OneOverWidth = 1.0f / (float)SaveDimX;
	float OneOverHeight = 1.0f / (float)SaveDimY;
    
	__m128 mmOneOverWidth = _mm_set1_ps(OneOverWidth);
	__m128 mmOneOverHeight = _mm_set1_ps(OneOverHeight);
    
	__m128i mmFF = _mm_set1_epi32(0xFF);
    
	for (int VertIndex = MinY; VertIndex < MaxY; VertIndex++) {
        
		float DeltaV = (float)(VertIndex - SaveMinY) * OneOverHeight;
        
        __m128 mmDeltaV = _mm_set1_ps(DeltaV);
        
        __m128 mmOutColor_r = _mm_add_ps(_mm_mul_ps(mmDeltaV, mmColorDiff_r), mmColor1_r);
        __m128 mmOutColor_g = _mm_add_ps(_mm_mul_ps(mmDeltaV, mmColorDiff_g), mmColor1_g);
        __m128 mmOutColor_b = _mm_add_ps(_mm_mul_ps(mmDeltaV, mmColorDiff_b), mmColor1_b);
        __m128 mmOutColor_a = mmOne;
        
        __m128i mmOutColorShifted_r = _mm_cvtps_epi32(_mm_mul_ps(mmOutColor_r, mm255));
        __m128i mmOutColorShifted_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_g, mm255)), 8);
        __m128i mmOutColorShifted_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_b, mm255)), 16);
        __m128i mmOutColorShifted_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmOutColor_a, mm255)), 24);
        
        __m128i mmOutColor = _mm_or_si128(
            _mm_or_si128(
            mmOutColorShifted_a, 
            mmOutColorShifted_b),
            _mm_or_si128(
            mmOutColorShifted_g, 
            mmOutColorShifted_r));
        
        
		for (int HorzIndex = MinX; HorzIndex < MaxX; HorzIndex += 4) {
			
			u32* Pixel = (u32*)((u8*)buf->pixels + VertIndex * buf->width * 4 + HorzIndex * 4);
			
			
			_mm_storeu_si128((__m128i*)Pixel, mmOutColor);
		}
	}
}


void RenderBitmapSSE(
Bmp_Info* buf,
Bmp_Info* bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 modulationColor01,
rc2 clipRect)
{
	float TargetScaling = (float)TargetBitmapPixelHeight / (float)bitmap->height;
	u32 TargetWidth = (float)bitmap->width * TargetScaling;
	u32 TargetHeight = TargetBitmapPixelHeight;
    
	__m128 mmTargetScaling = _mm_set1_ps(TargetScaling);
	__m128 mmTargetWidth = _mm_set1_ps((float)TargetWidth);
	__m128 mmTargetHeight = _mm_set1_ps(TargetBitmapPixelHeight);
    
	__m128 mmZero = _mm_set1_ps(0.0f);
	__m128 mmOne = _mm_set1_ps(1.0f);
	__m128i mmOneI = _mm_set1_epi32(1);
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
    
	__m128 mmSourceWidth = _mm_set1_ps((float)bitmap->width);
	__m128 mmSourceHeight = _mm_set1_ps((float)bitmap->height);
	__m128i mmSourcePitch = _mm_set1_epi32(bitmap->pitch);
    
	__m128 mmDestWidth = _mm_set1_ps((float)buf->width);
	__m128i mmDestWidthI = _mm_set1_epi32(buf->width);
    
	__m128i mmSourceWidthI = _mm_set1_epi32(bitmap->width);
	__m128i mmSourceHeightI = _mm_set1_epi32(bitmap->height);
    
	__m128i mmSourceWidthMinusOneI = _mm_sub_epi32(mmSourceWidthI, mmOneI);
	__m128i mmSourceHeightMinusOneI = _mm_sub_epi32(mmSourceHeightI, mmOneI);
    
	__m128 mmOneOverSourceWidth = _mm_set1_ps(1.0f / (float)bitmap->width);
	__m128 mmOneOverSourceHeight = _mm_set1_ps(1.0f / (float)bitmap->height);
    
	__m128 mmOneOverWidth = _mm_div_ps(mmOne, mmTargetWidth);
	__m128 mmOneOverHeight = _mm_div_ps(mmOne, mmTargetHeight);
    
	__m128i mmInitX = _mm_cvttps_epi32(_mm_set1_ps(P.x));
	__m128i mmInitY = _mm_cvttps_epi32(_mm_set1_ps(P.y));
    
	int InitX = P.x;
	int InitY = P.y;
    
	int MinX = InitX;
	int MaxX = MinX + TargetWidth;
    
	int MinY = InitY;
	int MaxY = MinY + TargetHeight;
    
    
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	__m128 mmmodulationColor_r = _mm_set1_ps(modulationColor01.r);
	__m128 mmmodulationColor_g = _mm_set1_ps(modulationColor01.g);
	__m128 mmmodulationColor_b = _mm_set1_ps(modulationColor01.b);
	__m128 mmmodulationColor_a = _mm_set1_ps(modulationColor01.a);
    
	mmmodulationColor_r = _mm_mul_ps(mmmodulationColor_r, mmmodulationColor_a);
	mmmodulationColor_g = _mm_mul_ps(mmmodulationColor_g, mmmodulationColor_a);
	mmmodulationColor_b = _mm_mul_ps(mmmodulationColor_b, mmmodulationColor_a);
    
	mmmodulationColor_r = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmmodulationColor_r));
	mmmodulationColor_g = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmmodulationColor_g));
	mmmodulationColor_b = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmmodulationColor_b));
	mmmodulationColor_a = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmmodulationColor_a));
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
        
		__m128i mmDestY = _mm_set1_epi32(DestY);
		__m128 mmPixelV = _mm_mul_ps(_mm_cvtepi32_ps(_mm_sub_epi32(mmDestY, mmInitY)), mmOneOverHeight);
		mmPixelV = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmPixelV));
		__m128 mmSourceY = _mm_mul_ps(mmPixelV, mmSourceHeight);
        
		for (int DestX = MinX; DestX < MaxX; DestX += 4) {
            
			__m128i mmDestX = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
			__m128 mmPixelU = _mm_mul_ps(_mm_cvtepi32_ps(_mm_sub_epi32(mmDestX, mmInitX)), mmOneOverWidth);
			mmPixelU = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmPixelU));
			__m128 mmSourceX = _mm_mul_ps(mmPixelU, mmSourceWidth);
            
			__m128i mmMinSourceX_ =  _mm_cvttps_epi32(mmSourceX);
			__m128i mmMinSourceY_ =  _mm_cvttps_epi32(mmSourceY);
            
			/*This check is for min values. If min value overlap then we clamp it*/
			mmMinSourceX_ = _mm_min_epi32(mmMinSourceX_, mmSourceWidthMinusOneI);
			mmMinSourceY_ = _mm_min_epi32(mmMinSourceY_, mmSourceHeightMinusOneI);
            
			__m128 mmDeltaX = _mm_sub_ps(mmSourceX, _mm_cvtepi32_ps(mmMinSourceX_));
			__m128 mmDeltaY = _mm_sub_ps(mmSourceY, _mm_cvtepi32_ps(mmMinSourceY_));
			
			/*Here we clamp max value*/
			__m128i mmMaxSourceX_ = _mm_min_epi32(_mm_add_epi32(mmMinSourceX_, mmOneI), mmSourceWidthMinusOneI);
			__m128i mmMaxSourceY_ = _mm_min_epi32(_mm_add_epi32(mmMinSourceY_, mmOneI), mmSourceHeightMinusOneI);
            
			__m128i mmMinSrcXx4 = _mm_slli_epi32(mmMinSourceX_, 2);
			__m128i mmMaxSrcXx4 = _mm_slli_epi32(mmMaxSourceX_, 2);
            
			__m128i mmPitchByMinSrcY = _mm_or_si128(_mm_mullo_epi16(mmMinSourceY_, mmSourcePitch), _mm_slli_epi32(_mm_mulhi_epi16(mmMinSourceY_, mmSourcePitch), 16));
			__m128i mmPitchByMaxSrcY = _mm_or_si128(_mm_mullo_epi16(mmMaxSourceY_, mmSourcePitch), _mm_slli_epi32(_mm_mulhi_epi16(mmMaxSourceY_, mmSourcePitch), 16));
            
			__m128i mmTopLTexelSrcOffset = _mm_add_epi32(mmPitchByMinSrcY, mmMinSrcXx4);
			__m128i mmTopRTexelSrcOffset = _mm_add_epi32(mmPitchByMinSrcY, mmMaxSrcXx4);
			__m128i mmBotLTexelSrcOffset = _mm_add_epi32(mmPitchByMaxSrcY, mmMinSrcXx4);
			__m128i mmBotRTexelSrcOffset = _mm_add_epi32(mmPitchByMaxSrcY, mmMaxSrcXx4);
            
			__m128i mmTopLTexel = _mm_setr_epi32(
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopLTexelSrcOffset, 0)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopLTexelSrcOffset, 1)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopLTexelSrcOffset, 2)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopLTexelSrcOffset, 3)));
            
			__m128i mmTopRTexel = _mm_setr_epi32(
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopRTexelSrcOffset, 0)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopRTexelSrcOffset, 1)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopRTexelSrcOffset, 2)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmTopRTexelSrcOffset, 3)));
            
			__m128i mmBotLTexel = _mm_setr_epi32(
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotLTexelSrcOffset, 0)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotLTexelSrcOffset, 1)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotLTexelSrcOffset, 2)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotLTexelSrcOffset, 3)));
            
			__m128i mmBotRTexel = _mm_setr_epi32(
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotRTexelSrcOffset, 0)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotRTexelSrcOffset, 1)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotRTexelSrcOffset, 2)),
				*(u32*)((u8*)bitmap->pixels + MMI(mmBotRTexelSrcOffset, 3)));
            
			__m128 mmTopLeft_r = MM_UNPACK_COLOR_CHANNEL0(mmTopLTexel);
			__m128 mmTopLeft_g = MM_UNPACK_COLOR_CHANNEL(mmTopLTexel, 8);
			__m128 mmTopLeft_b = MM_UNPACK_COLOR_CHANNEL(mmTopLTexel, 16);
			__m128 mmTopLeft_a = MM_UNPACK_COLOR_CHANNEL(mmTopLTexel, 24);
            
			__m128 mmTopRight_r = MM_UNPACK_COLOR_CHANNEL0(mmTopRTexel);
			__m128 mmTopRight_g = MM_UNPACK_COLOR_CHANNEL(mmTopRTexel, 8);
			__m128 mmTopRight_b = MM_UNPACK_COLOR_CHANNEL(mmTopRTexel, 16);
			__m128 mmTopRight_a = MM_UNPACK_COLOR_CHANNEL(mmTopRTexel, 24);
            
			__m128 mmBotLeft_r = MM_UNPACK_COLOR_CHANNEL0(mmBotLTexel);
			__m128 mmBotLeft_g = MM_UNPACK_COLOR_CHANNEL(mmBotLTexel, 8);
			__m128 mmBotLeft_b = MM_UNPACK_COLOR_CHANNEL(mmBotLTexel, 16);
			__m128 mmBotLeft_a = MM_UNPACK_COLOR_CHANNEL(mmBotLTexel, 24);
            
			__m128 mmBotRight_r = MM_UNPACK_COLOR_CHANNEL0(mmBotRTexel);
			__m128 mmBotRight_g = MM_UNPACK_COLOR_CHANNEL(mmBotRTexel, 8);
			__m128 mmBotRight_b = MM_UNPACK_COLOR_CHANNEL(mmBotRTexel, 16);
			__m128 mmBotRight_a = MM_UNPACK_COLOR_CHANNEL(mmBotRTexel, 24);
            
			/*First horizontal row blend*/
			__m128 mmUpperBlend_r = MM_LERP(mmTopLeft_r, mmTopRight_r, mmDeltaX);
			__m128 mmUpperBlend_g = MM_LERP(mmTopLeft_g, mmTopRight_g, mmDeltaX);
			__m128 mmUpperBlend_b = MM_LERP(mmTopLeft_b, mmTopRight_b, mmDeltaX);
			__m128 mmUpperBlend_a = MM_LERP(mmTopLeft_a, mmTopRight_a, mmDeltaX);
            
			/*Second horizontal row blend*/
			__m128 mmLowerBlend_r = MM_LERP(mmBotLeft_r, mmBotRight_r, mmDeltaX);
			__m128 mmLowerBlend_g = MM_LERP(mmBotLeft_g, mmBotRight_g, mmDeltaX);
			__m128 mmLowerBlend_b = MM_LERP(mmBotLeft_b, mmBotRight_b, mmDeltaX);
			__m128 mmLowerBlend_a = MM_LERP(mmBotLeft_a, mmBotRight_a, mmDeltaX);
            
			/*Vertical blend*/
			__m128 mmBlended_r = MM_LERP(mmUpperBlend_r, mmLowerBlend_r, mmDeltaY);
			__m128 mmBlended_g = MM_LERP(mmUpperBlend_g, mmLowerBlend_g, mmDeltaY);
			__m128 mmBlended_b = MM_LERP(mmUpperBlend_b, mmLowerBlend_b, mmDeltaY);
			__m128 mmBlended_a = MM_LERP(mmUpperBlend_a, mmLowerBlend_a, mmDeltaY);
            
			/*Multiplication by input color*/
			mmBlended_r = _mm_mul_ps(mmBlended_r, mmmodulationColor_r);
			mmBlended_g = _mm_mul_ps(mmBlended_g, mmmodulationColor_g);
			mmBlended_b = _mm_mul_ps(mmBlended_b, mmmodulationColor_b);
			mmBlended_a = _mm_mul_ps(mmBlended_a, mmmodulationColor_a);
            
			u32* OutDest = (u32*)((u8*)buf->pixels + DestY * buf->pitch + DestX * 4);
            
			__m128i mmPreDestColor = _mm_loadu_si128((__m128i*)OutDest);
            
			__m128 mmPreDestColor_r = MM_UNPACK_COLOR_CHANNEL0(mmPreDestColor);
			__m128 mmPreDestColor_g = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 8);
			__m128 mmPreDestColor_b = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 16);
			__m128 mmPreDestColor_a = MM_UNPACK_COLOR_CHANNEL(mmPreDestColor, 24);
            
			__m128 mmBlendAlpha = mmBlended_a;
            
			/*Final alpha blend*/
			__m128 mmOneMinusBlendAlpha = _mm_sub_ps(mmOne, mmBlendAlpha);
			__m128 mmColor_r = _mm_add_ps(_mm_mul_ps(mmPreDestColor_r, mmOneMinusBlendAlpha), mmBlended_r);
			__m128 mmColor_g = _mm_add_ps(_mm_mul_ps(mmPreDestColor_g, mmOneMinusBlendAlpha), mmBlended_g);
			__m128 mmColor_b = _mm_add_ps(_mm_mul_ps(mmPreDestColor_b, mmOneMinusBlendAlpha), mmBlended_b);
			//__m128 mmColor_a = mmOne;
			__m128 mmColor_a = _mm_sub_ps(_mm_add_ps(mmPreDestColor_a, mmBlended_a), _mm_mul_ps(mmPreDestColor_a, mmBlended_a));
            
			__m128i mmColorShifted_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmColor_a, mm255)), 24);
			__m128i mmColorShifted_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmColor_b, mm255)), 16);
			__m128i mmColorShifted_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmColor_g, mm255)), 8);
			__m128i mmColorShifted_r = _mm_cvtps_epi32(_mm_mul_ps(mmColor_r, mm255));
            
			__m128i mmResult = _mm_or_si128(
				_mm_or_si128(mmColorShifted_r, mmColorShifted_g),
				_mm_or_si128(mmColorShifted_b, mmColorShifted_a));
            
			/*Mask with end of screen mask*/
			//__m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmDestX), mmDestWidth);
			__m128 mmEndScreenMask = _mm_cmplt_ps(_mm_cvtepi32_ps(mmDestX), mmMaxXF);
            
			mmResult = _mm_castps_si128(_mm_or_ps(
				_mm_and_ps(_mm_castsi128_ps(mmResult), mmEndScreenMask),
				_mm_andnot_ps(mmEndScreenMask, _mm_castsi128_ps(mmPreDestColor))));
            
			//TODO(DIma): make this aligned
			_mm_storeu_si128((__m128i*)OutDest, mmResult);
		}
	}
    
	u32 PixelFillCount = (MaxY - MinY) * (MaxX - MinX);
}

void RenderBitmap(
Bmp_Info* buf,
Bmp_Info* bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 modulationColor01, 
rc2 clipRect)
{
	float TargetScaling = (float)TargetBitmapPixelHeight / (float)bitmap->height;
    
	modulationColor01.r *= modulationColor01.a;
	modulationColor01.g *= modulationColor01.a;
	modulationColor01.b *= modulationColor01.a;
    
	/*Clamping incoming color*/
	modulationColor01.r = Clamp01(modulationColor01.r);
	modulationColor01.g = Clamp01(modulationColor01.g);
	modulationColor01.b = Clamp01(modulationColor01.b);
	modulationColor01.a = Clamp01(modulationColor01.a);
    
	u32 TargetWidth = (float)bitmap->width * TargetScaling;
	u32 TargetHeight = TargetBitmapPixelHeight;
    
	int InitX = P.x;
	int InitY = P.y;
    
	int MinX = InitX;
	int MaxX = MinX + TargetWidth;
    
	int MinY = InitY;
	int MaxY = MinY + TargetHeight;
    
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	float SourceWidth = bitmap->width;
	float SourceHeight = bitmap->height;
    
	float OneOverSrcWidth = 1.0f / SourceWidth;
	float OneOverSrcHeight = 1.0f / SourceHeight;
    
	float OneOverWidth = 1.0f / (float)TargetWidth;
	float OneOverHeight = 1.0f / (float)TargetHeight;
    
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
        
		float PixelV = ((float)DestY - (float)InitY) * OneOverHeight;
		PixelV = Clamp01(PixelV);
		float SourceY = PixelV * SourceHeight;
        
		for (int DestX = MinX; DestX < MaxX; DestX++) {
            
			float PixelU = ((float)DestX - (float)InitX) * OneOverWidth;
			PixelU = Clamp01(PixelU);
			float SourceX = PixelU * SourceWidth;
            
			u32 MinSourceX_ = (u32)SourceX;
			u32 MinSourceY_ = (u32)SourceY;
            
			float DeltaX = SourceX - (float)MinSourceX_;
			float DeltaY = SourceY - (float)MinSourceY_;
            
			u32 MaxSourceX_ = Min(MinSourceX_ + 1, bitmap->width - 1);
			u32 MaxSourceY_ = Min(MinSourceY_ + 1, bitmap->height - 1);
            
			u32* TopLeft = (u32*)((u8*)bitmap->pixels + bitmap->pitch * MinSourceY_ + MinSourceX_ * 4);
			u32* TopRight = (u32*)((u8*)bitmap->pixels + bitmap->pitch * MinSourceY_ + MaxSourceX_ * 4);
			u32* BotLeft = (u32*)((u8*)bitmap->pixels + bitmap->pitch * MaxSourceY_ + MinSourceX_ * 4);
			u32* BotRight = (u32*)((u8*)bitmap->pixels + bitmap->pitch * MaxSourceY_ + MaxSourceX_ * 4);
            
			v4 TopLeftColor = UnpackRGBA(*TopLeft);
			v4 TopRightColor = UnpackRGBA(*TopRight);
			v4 BotLeftColor = UnpackRGBA(*BotLeft);
			v4 BotRightColor = UnpackRGBA(*BotRight);
            
			/*First row blend*/
			v4 UpperBlend;
			UpperBlend.r = TopLeftColor.r + (TopRightColor.r - TopLeftColor.r) * DeltaX;
			UpperBlend.g = TopLeftColor.g + (TopRightColor.g - TopLeftColor.g) * DeltaX;
			UpperBlend.b = TopLeftColor.b + (TopRightColor.b - TopLeftColor.b) * DeltaX;
			UpperBlend.a = TopLeftColor.a + (TopRightColor.a - TopLeftColor.a) * DeltaX;
            
			/*Second row blend*/
			v4 LowerBlend;
			LowerBlend.r = BotLeftColor.r + (BotRightColor.r - BotLeftColor.r) * DeltaX;
			LowerBlend.g = BotLeftColor.g + (BotRightColor.g - BotLeftColor.g) * DeltaX;
			LowerBlend.b = BotLeftColor.b + (BotRightColor.b - BotLeftColor.b) * DeltaX;
			LowerBlend.a = BotLeftColor.a + (BotRightColor.a - BotLeftColor.a) * DeltaX;
            
			/*Vertical blend*/
			v4 BlendedColor;
			BlendedColor.r = UpperBlend.r + (LowerBlend.r - UpperBlend.r) * DeltaY;
			BlendedColor.g = UpperBlend.g + (LowerBlend.g - UpperBlend.g) * DeltaY;
			BlendedColor.b = UpperBlend.b + (LowerBlend.b - UpperBlend.b) * DeltaY;
			BlendedColor.a = UpperBlend.a + (LowerBlend.a - UpperBlend.a) * DeltaY;
			
			BlendedColor.r = BlendedColor.r * modulationColor01.r;
			BlendedColor.g = BlendedColor.g * modulationColor01.g;
			BlendedColor.b = BlendedColor.b * modulationColor01.b;
			BlendedColor.a = BlendedColor.a * modulationColor01.a;
            
			u32* OutDest = (u32*)((u8*)buf->pixels + DestY * buf->pitch + DestX * 4);
			v4 PreDestColor = UnpackRGBA(*OutDest);
            
			//float BlendAlpha = PreDestColor.a + BlendedColor.a - PreDestColor.a * BlendedColor.a;
			float BlendAlpha = BlendedColor.a;
			//Assert((BlendAlpha >= 0.0f) && (BlendAlpha <= 1.0f));
            
			/*Premultiplied alpha in action*/
			v4 AlphaBlendColor;
			AlphaBlendColor.r = (1.0f - BlendAlpha) * PreDestColor.r + BlendedColor.r;
			AlphaBlendColor.g = (1.0f - BlendAlpha) * PreDestColor.g + BlendedColor.g;
			AlphaBlendColor.b = (1.0f - BlendAlpha) * PreDestColor.b + BlendedColor.b;
			AlphaBlendColor.a = PreDestColor.a + BlendedColor.a - PreDestColor.a * BlendedColor.a;
            
			u32 DestPackedColor = PackRGBA(AlphaBlendColor);
			*OutDest = DestPackedColor;
		}
	}
    
	u32 PixelFillCount = (MaxY - MinY) * (MaxX - MinX);
}

void RenderRect(
Bmp_Info* buf,
v2 P,
v2 Dim,
v4 modulationColor01, 
rc2 clipRect)
{
	int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	/*Clamping incoming color*/
	modulationColor01.r = Clamp01(modulationColor01.r);
	modulationColor01.g = Clamp01(modulationColor01.g);
	modulationColor01.b = Clamp01(modulationColor01.b);
	modulationColor01.a = Clamp01(modulationColor01.a);
    
	/*Premultiplying incoming color with it alpha*/
	modulationColor01.r *= modulationColor01.a;
	modulationColor01.g *= modulationColor01.a;
	modulationColor01.b *= modulationColor01.a;
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
		for (int DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)((u8*)buf->pixels + DestY * buf->pitch + DestX * 4);
            
			v4 ResultColor = modulationColor01;
			v4 PreDestColor = UnpackRGBA(*OutDest);
            
			float BlendAlpha = modulationColor01.a;
            
			v4 AlphaBlendColor;
			AlphaBlendColor.r = (1.0f - BlendAlpha) * PreDestColor.r + ResultColor.r;
			AlphaBlendColor.g = (1.0f - BlendAlpha) * PreDestColor.g + ResultColor.g;
			AlphaBlendColor.b = (1.0f - BlendAlpha) * PreDestColor.b + ResultColor.b;
			AlphaBlendColor.a = PreDestColor.a + ResultColor.a - PreDestColor.a * ResultColor.a;
            
			u32 DestPackedColor = PackRGBA(AlphaBlendColor);
            
			*OutDest = DestPackedColor;
		}
	}
}

void RenderRectSSE(
Bmp_Info* buf,
v2 P,
v2 Dim,
v4 modulationColor01,
rc2 clipRect)
{
	int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
	/*Clamping incoming color*/
	MinX = Clamp(MinX, 0, buf->width);
	MaxX = Clamp(MaxX, 0, buf->width);
	MinY = Clamp(MinY, 0, buf->height);
	MaxY = Clamp(MaxY, 0, buf->height);
    
	MinX = Clamp(MinX, (int)clipRect.min.x, (int)clipRect.max.x);
	MinY = Clamp(MinY, (int)clipRect.min.y, (int)clipRect.max.y);
	MaxX = Clamp(MaxX, (int)clipRect.min.x, (int)clipRect.max.x);
	MaxY = Clamp(MaxY, (int)clipRect.min.y, (int)clipRect.max.y);
    
	__m128 mmOne = _mm_set1_ps(1.0f);
	__m128 mmZero = _mm_set1_ps(0.0f);
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	__m128 mmModColor_r = _mm_set1_ps(modulationColor01.r);
	__m128 mmModColor_g = _mm_set1_ps(modulationColor01.g);
	__m128 mmModColor_b = _mm_set1_ps(modulationColor01.b);
	__m128 mmModColor_a = _mm_set1_ps(modulationColor01.a);
    
	mmModColor_r = _mm_min_ps(_mm_max_ps(mmModColor_r, mmZero), mmOne);
	mmModColor_g = _mm_min_ps(_mm_max_ps(mmModColor_g, mmZero), mmOne);
	mmModColor_b = _mm_min_ps(_mm_max_ps(mmModColor_b, mmZero), mmOne);
	mmModColor_a = _mm_min_ps(_mm_max_ps(mmModColor_a, mmZero), mmOne);
    
	/*Premultiplying incoming color with it alpha*/
	mmModColor_r = _mm_mul_ps(_mm_mul_ps(mmModColor_r, mmModColor_a), mm255);
	mmModColor_g = _mm_mul_ps(_mm_mul_ps(mmModColor_g, mmModColor_a), mm255);
	mmModColor_b = _mm_mul_ps(_mm_mul_ps(mmModColor_b, mmModColor_a), mm255);
    
	__m128i mmDestWidth = _mm_set1_epi32(buf->width);
	__m128 mmDestWidthF = _mm_cvtepi32_ps(mmDestWidth);
    
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
		for (int DestX = MinX; DestX < MaxX; DestX += 4) {
			__m128i mmDestX = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
			__m128 mmDestXF = _mm_cvtepi32_ps(mmDestX);
            
			u32* OutDest = (u32*)((u8*)buf->pixels + DestY * buf->pitch + (DestX << 2));
			__m128i mmDstPixels = _mm_loadu_si128((__m128i*)OutDest);
            
#if 0
			__m128 mmPreDestColor_r = _mm_cvtepi32_ps(_mm_and_si128(mmDstPixels, mmFF));
            __m128 mmPreDestColor_g = 
                _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 8), mmFF));
			__m128 mmPreDestColor_b = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 16), mmFF));
			__m128 mmPreDestColor_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 24), mmFF));
			
			mmPreDestColor_r = _mm_mul_ps(mmOneOver255, mmPreDestColor_r);
			mmPreDestColor_g = _mm_mul_ps(mmOneOver255, mmPreDestColor_g);
			mmPreDestColor_b = _mm_mul_ps(mmOneOver255, mmPreDestColor_b);
			mmPreDestColor_a = _mm_mul_ps(mmOneOver255, mmPreDestColor_a);
            
			__m128 mmOneMinusT = _mm_sub_ps(mmOne, mmModColor_a);
			__m128 mmAlphaBlend_r = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_r), mmModColor_r);
			__m128 mmAlphaBlend_g = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_g), mmModColor_g);
			__m128 mmAlphaBlend_b = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_b), mmModColor_b);
			__m128 mmAlphaBlend_a = _mm_sub_ps(_mm_add_ps(mmPreDestColor_a, mmModColor_a), _mm_mul_ps(mmPreDestColor_a, mmModColor_a));
			
			__m128i mmResult_r = _mm_cvtps_epi32(_mm_mul_ps(mmAlphaBlend_r, mm255));
            __m128i mmResult_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmAlphaBlend_g, mm255)), 8);
			__m128i mmResult_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmAlphaBlend_b, mm255)), 16);
			__m128i mmResult_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmAlphaBlend_a, mm255)), 24);
#else
			__m128 mmPreDestColor_r = _mm_cvtepi32_ps(_mm_and_si128(mmDstPixels, mmFF));
            __m128 mmPreDestColor_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 8), mmFF));
			__m128 mmPreDestColor_b = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 16), mmFF));
			__m128 mmPreDestColor_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(mmDstPixels, 24), mmFF));
			
			mmPreDestColor_a = _mm_mul_ps(mmOneOver255, mmPreDestColor_a);
            
			__m128 mmOneMinusT = _mm_sub_ps(mmOne, mmModColor_a);
			__m128 mmAlphaBlend_r = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_r), mmModColor_r);
			__m128 mmAlphaBlend_g = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_g), mmModColor_g);
			__m128 mmAlphaBlend_b = _mm_add_ps(_mm_mul_ps(mmOneMinusT, mmPreDestColor_b), mmModColor_b);
			__m128 mmAlphaBlend_a = _mm_sub_ps(_mm_add_ps(mmPreDestColor_a, mmModColor_a), _mm_mul_ps(mmPreDestColor_a, mmModColor_a));
            
			__m128i mmResult_r = _mm_cvtps_epi32(mmAlphaBlend_r);
            __m128i mmResult_g = _mm_slli_epi32(_mm_cvtps_epi32(mmAlphaBlend_g), 8);
			__m128i mmResult_b = _mm_slli_epi32(_mm_cvtps_epi32(mmAlphaBlend_b), 16);
			__m128i mmResult_a = _mm_slli_epi32(_mm_cvtps_epi32(mmAlphaBlend_a), 24);
#endif
            
			__m128i mmResult = _mm_or_si128(_mm_or_si128(mmResult_r, mmResult_g), _mm_or_si128(mmResult_b, mmResult_a));
            
			/*Add end screen mask*/
			__m128 mmEndScreenMask = _mm_cmplt_ps(mmDestXF, mmDestWidthF);
			__m128 mmWidthMask = _mm_cmplt_ps(mmDestXF, mmMaxXF);
			__m128 mmMask = _mm_and_ps(mmWidthMask, mmEndScreenMask);
            
			mmResult = _mm_castps_si128(_mm_or_ps(
				_mm_and_ps(_mm_castsi128_ps(mmResult), mmMask),
				_mm_andnot_ps(mmMask, _mm_castsi128_ps(mmDstPixels))));
            
			_mm_storeu_si128((__m128i*)OutDest, mmResult);
		}
	}
    
	u32 PixelFillCount = (MaxY - MinY) * (MaxX - MinX);
}

void SoftwareRenderStackToOutput(Render_Stack* stack, Bmp_Info* buf, rc2 clipRect){
    u8* at = (u8*)stack->mem;
	u8* stackEnd = (u8*)stack->mem + stack->memUsed;
    
	while (at < stackEnd) {
        Render_Entry_Header* header = (Render_Entry_Header*)at;
        
        at += sizeof(Render_Entry_Header);
        
        switch(header->type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(RenderEntryClearColor);
                
                RenderClearSSE(buf, entry->clearColor01, clipRect);
            }break;
            
            case RenderEntry_Gradient:{
                RENDER_GET_ENTRY(RenderEntryGradient);
                
                if(entry->gradType == RenderEntryGradient_Horizontal){
                    RenderGradientHorzSSE(buf, 
                                          entry->rc, 
                                          entry->color1, 
                                          entry->color2, 
                                          clipRect);
                }
                else if(entry->gradType == RenderEntryGradient_Vertical){
                    RenderGradientVertSSE(buf, 
                                          entry->rc, 
                                          entry->color1, 
                                          entry->color2, 
                                          clipRect);
                }
                else{
                    INVALID_CODE_PATH;
                }
            }break;
            
            case RenderEntry_Rect:{
                RENDER_GET_ENTRY(RenderEntryRect);
                
#if 1
                RenderRectSSE(buf,
                              entry->p,
                              entry->dim,
                              entry->modulationColor01,
                              clipRect);
#else
                RenderRect(buf,
                           entry->p,
                           entry->dim,
                           entry->modulationColor01,
                           clipRect);
#endif
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(RenderEntryBitmap);
                
#if 1
                RenderBitmapSSE(buf, 
                                entry->bitmap,
                                entry->topLeftP,
                                entry->pixelHeight,
                                entry->modulationColor01,
                                clipRect);
#else
                RenderBitmap(buf, 
                             entry->bitmap,
                             entry->topLeftP,
                             entry->pixelHeight,
                             entry->modulationColor01,
                             clipRect);
#endif
            }break;
            
            case RenderEntry_Glyph:{
                
            }break;
        }
        
        at += header->dataSize;
    }
}

struct Render_Queue_Work_Data{
    Bmp_Info* buf;
    rc2 clipRect;
    Render_Stack* stack;
};

PLATFORM_CALLBACK(RenderQueueWork){
    Render_Queue_Work_Data* work = (Render_Queue_Work_Data*)data;
    
    SoftwareRenderStackToOutput(
        work->stack,
        work->buf,
        work->clipRect);
}

struct Render_Queue_rgba2bgra_Work{
    Bmp_Info* buf;
    rc2 clipRect;
};

PLATFORM_CALLBACK(RenderQueueRGBA2BGRAWork){
    Render_Queue_rgba2bgra_Work* work = (Render_Queue_rgba2bgra_Work*)data;
    
    RenderRGBA2BGRASSE(work->buf, work->clipRect);
}

#define TILES_COUNT 32
void RenderMultithreaded(Platform_Job_Queue* queue, Render_Stack* stack, Bmp_Info* buf) {
    
#if 0
	rc2 clipRect;
	clipRect.min = V2(0, 0);
	clipRect.max = V2(buf->width, buf->height);
    
	SoftwareRenderStackToOutput(stack, buf, clipRect);
#else
    
#if 0
#define SIDE_TILES_COUNT 8
    
	RenderQueueWork works[SIDE_TILES_COUNT * SIDE_TILES_COUNT];
    
	int sideTileW = buf->width / SIDE_TILES_COUNT;
    sideTileW = (sideTileW + 15) & (~15);
	int sideTileH = buf->height / SIDE_TILES_COUNT;
    
    for (int j = 0; j < SIDE_TILES_COUNT; j++) {
		for (int i = 0; i < SIDE_TILES_COUNT; i++) {
            rc2 rect;
            
            rect.min = V2(sideTileW * i, sideTileH * j);
            rect.max = V2(sideTileW * (i + 1), sideTileH * (j + 1));
            
            if (j == SIDE_TILES_COUNT - 1) {
                rect.max.y = buf->height;
            }
            
            if (i == SIDE_TILES_COUNT - 1) {
                rect.max.x = buf->width;
            }
            
            RenderQueueWork* workData = &works[j * SIDE_TILES_COUNT + i];
            workData->buf = buf;
            workData->stack = stack;
            workData->clipRect = rect;
            
            //if ((j & 1) == (i & 1)) {
            PlatformAddEntry(Queue, RenderQueueWork, workData);
            //}
		}
    }
#else
    Render_Queue_Work_Data works[TILES_COUNT];
    
    int preferTileH = buf->height / TILES_COUNT;
    if(preferTileH < 16){
        preferTileH = 16;
    }
    
    int curH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 rect;
        
        b32 shouldExit = 0;
        int maxH = curH + preferTileH;
        if(maxH > buf->height){
            maxH = buf->height;
            shouldExit = 1;
        }
        
        rect.min = V2(0.0f, curH);
        rect.max = V2(buf->width, maxH);
        
        Render_Queue_Work_Data* workData = &works[i];
        workData->buf = buf;
        workData->stack = stack;
        workData->clipRect = rect;
        
        //if (i & 1) {
        PlatformAddEntry(queue, RenderQueueWork, workData);
        //}
        
        curH += preferTileH;
        if(shouldExit){
            break;
        }
    }
#endif
    
    PlatformWaitForCompletion(queue);
#endif
}

void RenderMultithreadedRGBA2BGRA(Platform_Job_Queue* queue, Bmp_Info* buf) {
    
    Render_Queue_rgba2bgra_Work works[TILES_COUNT];
    
#if 1
    int preferTileH = buf->height / TILES_COUNT;
    if(preferTileH < 16){
        preferTileH = 16;
    }
    
    int currentH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 rect;
        
        b32 shouldExit = 0;
        int maxH = currentH + preferTileH;
        if(maxH > buf->height){
            maxH = buf->height;
            shouldExit = 1;
        }
        
        rect.min = V2(0.0f, currentH);
        rect.max = V2(buf->width, maxH);
        
        Render_Queue_rgba2bgra_Work* workData = &works[i];
        workData->buf = buf;
        workData->clipRect = rect;
        
        //if (i & 1) {
        PlatformAddEntry(queue, RenderQueueRGBA2BGRAWork, workData);
        //}
        
        currentH += preferTileH;
        if(shouldExit){
            break;
        }
    }
    
    PlatformWaitForCompletion(queue);
#endif
    
}
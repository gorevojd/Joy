#include "joy_software_renderer.h"
#include "joy_render_stack.h"
#include "joy_platform.h"

#include <intrin.h>

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)
#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)
#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

void RenderOneBitmapIntoAnother(
bmp_info* RenderTo, 
bmp_info* RenderWhat,
int StartX,
int StartY,
v4 ModulationColor) 
{
	float OneOver255 = 1.0f / 255.0f;
    
	int MaxToX = StartX + RenderWhat->Width;
	int MaxToY = StartY + RenderWhat->Height;
    
	Assert(MaxToX <= RenderTo->Width);
	Assert(MaxToY <= RenderTo->Height);
    
	u32 SrcX = 0;
	u32 SrcY = 0;
    
	for (int Y = StartY; Y < MaxToY; Y++) {
		SrcY = Y - StartY;
        
		for (int X = StartX; X < MaxToX; X++) {
			SrcX = X - StartX;
            
			u32* Out = (u32*)RenderTo->Pixels + Y * RenderTo->Width + X;
			v4 DstInitColor = UnpackRGBA(*Out);
            
            
			u32* From = (u32*)RenderWhat->Pixels + SrcY * RenderWhat->Width + SrcX;
            
			v4 FromColor = UnpackRGBA(*From);
            
			v4 ResultColor = FromColor * ModulationColor;
            
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


void RenderRGBA2BGRA(bmp_info* Buffer, rc2 ClipRect){
	int MinX = 0;
	int MaxX = Buffer->Width;
	int MinY = 0;
	int MaxY = Buffer->Height;
    
    MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)Buffer->Pixels + DestY * Buffer->Width + DestX;
            
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


void RenderRGBA2BGRASSE(bmp_info* Buffer, rc2 ClipRect) {
	
	int MinX = 0;
	int MaxX = Buffer->Width;
	int MinY = 0;
	int MaxY = Buffer->Height;
    
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX += 4) {
			__m128i mmHorzIndex = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
            
            void* OutDest = (u8*)Buffer->Pixels + DestY * Buffer->Pitch + DestX * 4;
            
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


void RenderClear(bmp_info* Buffer, v3 Color, rc2 ClipRect) {
	v4 ResColor = V4(Color.x, Color.y, Color.z, 1.0f);
	u32 OutColor = PackRGBA(ResColor);
    
	int MinX = 0;
	int MaxX = Buffer->Width;
	int MinY = 0;
	int MaxY = Buffer->Height;
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	for (u32 DestY = MinY; DestY < MaxY; DestY++) {
		for (u32 DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)Buffer->Pixels + DestY * Buffer->Width + DestX;
            
			*OutDest = OutColor;
		}
	}
}


void RenderClearSSE(bmp_info* Buffer, v3 Color, rc2 ClipRect) {
	v4 ResColor = V4(Color.x, Color.y, Color.z, 1.0f);
	u32 OutColor = PackRGBA(ResColor);
    
	int MinX = 0;
	int MaxX = Buffer->Width;
	int MinY = 0;
	int MaxY = Buffer->Height;
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
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
			//u32* OutDest = (u32*)Buffer->Pixels + DestY * Buffer->Width + DestX;
            
#if 1
			__m128i mmHorzIndex = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
            
			u32* OutDest = (u32*)((u8*)Buffer->Pixels + DestY * Buffer->Pitch + DestX * 4);
            
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


void RenderBitmapSSE(
bmp_info* Buffer,
bmp_info* Bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 ModulationColor01,
rc2 ClipRect)
{
	float TargetScaling = (float)TargetBitmapPixelHeight / (float)Bitmap->Height;
	u32 TargetWidth = (float)Bitmap->Width * TargetScaling;
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
    
	__m128 mmSourceWidth = _mm_set1_ps((float)Bitmap->Width);
	__m128 mmSourceHeight = _mm_set1_ps((float)Bitmap->Height);
	__m128i mmSourcePitch = _mm_set1_epi32(Bitmap->Pitch);
    
	__m128 mmDestWidth = _mm_set1_ps((float)Buffer->Width);
	__m128i mmDestWidthI = _mm_set1_epi32(Buffer->Width);
    
	__m128i mmSourceWidthI = _mm_set1_epi32(Bitmap->Width);
	__m128i mmSourceHeightI = _mm_set1_epi32(Bitmap->Height);
    
	__m128i mmSourceWidthMinusOneI = _mm_sub_epi32(mmSourceWidthI, mmOneI);
	__m128i mmSourceHeightMinusOneI = _mm_sub_epi32(mmSourceHeightI, mmOneI);
    
	__m128 mmOneOverSourceWidth = _mm_set1_ps(1.0f / (float)Bitmap->Width);
	__m128 mmOneOverSourceHeight = _mm_set1_ps(1.0f / (float)Bitmap->Height);
    
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
    
    
	MinX = Clamp(MinX, 0, Buffer->Width);
	MaxX = Clamp(MaxX, 0, Buffer->Width);
	MinY = Clamp(MinY, 0, Buffer->Height);
	MaxY = Clamp(MaxY, 0, Buffer->Height);
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	__m128 mmModulationColor_r = _mm_set1_ps(ModulationColor01.r);
	__m128 mmModulationColor_g = _mm_set1_ps(ModulationColor01.g);
	__m128 mmModulationColor_b = _mm_set1_ps(ModulationColor01.b);
	__m128 mmModulationColor_a = _mm_set1_ps(ModulationColor01.a);
    
	mmModulationColor_r = _mm_mul_ps(mmModulationColor_r, mmModulationColor_a);
	mmModulationColor_g = _mm_mul_ps(mmModulationColor_g, mmModulationColor_a);
	mmModulationColor_b = _mm_mul_ps(mmModulationColor_b, mmModulationColor_a);
    
	mmModulationColor_r = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmModulationColor_r));
	mmModulationColor_g = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmModulationColor_g));
	mmModulationColor_b = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmModulationColor_b));
	mmModulationColor_a = _mm_min_ps(mmOne, _mm_max_ps(mmZero, mmModulationColor_a));
    
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
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopLTexelSrcOffset, 0)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopLTexelSrcOffset, 1)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopLTexelSrcOffset, 2)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopLTexelSrcOffset, 3)));
            
			__m128i mmTopRTexel = _mm_setr_epi32(
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopRTexelSrcOffset, 0)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopRTexelSrcOffset, 1)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopRTexelSrcOffset, 2)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmTopRTexelSrcOffset, 3)));
            
			__m128i mmBotLTexel = _mm_setr_epi32(
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotLTexelSrcOffset, 0)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotLTexelSrcOffset, 1)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotLTexelSrcOffset, 2)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotLTexelSrcOffset, 3)));
            
			__m128i mmBotRTexel = _mm_setr_epi32(
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotRTexelSrcOffset, 0)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotRTexelSrcOffset, 1)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotRTexelSrcOffset, 2)),
				*(u32*)((u8*)Bitmap->Pixels + MMI(mmBotRTexelSrcOffset, 3)));
            
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
			mmBlended_r = _mm_mul_ps(mmBlended_r, mmModulationColor_r);
			mmBlended_g = _mm_mul_ps(mmBlended_g, mmModulationColor_g);
			mmBlended_b = _mm_mul_ps(mmBlended_b, mmModulationColor_b);
			mmBlended_a = _mm_mul_ps(mmBlended_a, mmModulationColor_a);
            
			u32* OutDest = (u32*)((u8*)Buffer->Pixels + DestY * Buffer->Pitch + DestX * 4);
            
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
bmp_info* Buffer,
bmp_info* Bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 ModulationColor01, 
rc2 ClipRect)
{
	float TargetScaling = (float)TargetBitmapPixelHeight / (float)Bitmap->Height;
    
	ModulationColor01.r *= ModulationColor01.a;
	ModulationColor01.g *= ModulationColor01.a;
	ModulationColor01.b *= ModulationColor01.a;
    
	/*Clamping incoming color*/
	ModulationColor01.r = Clamp01(ModulationColor01.r);
	ModulationColor01.g = Clamp01(ModulationColor01.g);
	ModulationColor01.b = Clamp01(ModulationColor01.b);
	ModulationColor01.a = Clamp01(ModulationColor01.a);
    
	u32 TargetWidth = (float)Bitmap->Width * TargetScaling;
	u32 TargetHeight = TargetBitmapPixelHeight;
    
	int InitX = P.x;
	int InitY = P.y;
    
	int MinX = InitX;
	int MaxX = MinX + TargetWidth;
    
	int MinY = InitY;
	int MaxY = MinY + TargetHeight;
    
	MinX = Clamp(MinX, 0, Buffer->Width);
	MaxX = Clamp(MaxX, 0, Buffer->Width);
	MinY = Clamp(MinY, 0, Buffer->Height);
	MaxY = Clamp(MaxY, 0, Buffer->Height);
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	float SourceWidth = Bitmap->Width;
	float SourceHeight = Bitmap->Height;
    
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
            
			u32 MaxSourceX_ = Min(MinSourceX_ + 1, Bitmap->Width - 1);
			u32 MaxSourceY_ = Min(MinSourceY_ + 1, Bitmap->Height - 1);
            
			u32* TopLeft = (u32*)((u8*)Bitmap->Pixels + Bitmap->Pitch * MinSourceY_ + MinSourceX_ * 4);
			u32* TopRight = (u32*)((u8*)Bitmap->Pixels + Bitmap->Pitch * MinSourceY_ + MaxSourceX_ * 4);
			u32* BotLeft = (u32*)((u8*)Bitmap->Pixels + Bitmap->Pitch * MaxSourceY_ + MinSourceX_ * 4);
			u32* BotRight = (u32*)((u8*)Bitmap->Pixels + Bitmap->Pitch * MaxSourceY_ + MaxSourceX_ * 4);
            
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
			
			BlendedColor.r = BlendedColor.r * ModulationColor01.r;
			BlendedColor.g = BlendedColor.g * ModulationColor01.g;
			BlendedColor.b = BlendedColor.b * ModulationColor01.b;
			BlendedColor.a = BlendedColor.a * ModulationColor01.a;
            
			u32* OutDest = (u32*)((u8*)Buffer->Pixels + DestY * Buffer->Pitch + DestX * 4);
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
bmp_info* Buffer,
v2 P,
v2 Dim,
v4 ModulationColor01, 
rc2 ClipRect)
{
	int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
	MinX = Clamp(MinX, 0, Buffer->Width);
	MaxX = Clamp(MaxX, 0, Buffer->Width);
	MinY = Clamp(MinY, 0, Buffer->Height);
	MaxY = Clamp(MaxY, 0, Buffer->Height);
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	/*Clamping incoming color*/
	ModulationColor01.r = Clamp01(ModulationColor01.r);
	ModulationColor01.g = Clamp01(ModulationColor01.g);
	ModulationColor01.b = Clamp01(ModulationColor01.b);
	ModulationColor01.a = Clamp01(ModulationColor01.a);
    
	/*Premultiplying incoming color with it alpha*/
	ModulationColor01.r *= ModulationColor01.a;
	ModulationColor01.g *= ModulationColor01.a;
	ModulationColor01.b *= ModulationColor01.a;
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
		for (int DestX = MinX; DestX < MaxX; DestX++) {
			u32* OutDest = (u32*)((u8*)Buffer->Pixels + DestY * Buffer->Pitch + DestX * 4);
            
			v4 ResultColor = ModulationColor01;
			v4 PreDestColor = UnpackRGBA(*OutDest);
            
			float BlendAlpha = ModulationColor01.a;
            
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
bmp_info* Buffer,
v2 P,
v2 Dim,
v4 ModulationColor01,
rc2 ClipRect)
{
	int InitX = floorf(P.x);
	int InitY = floorf(P.y);
    
	int MinX = InitX;
	int MaxX = MinX + ceilf(Dim.x);
    
	int MinY = InitY;
	int MaxY = MinY + ceilf(Dim.y);
    
	/*Clamping incoming color*/
	MinX = Clamp(MinX, 0, Buffer->Width);
	MaxX = Clamp(MaxX, 0, Buffer->Width);
	MinY = Clamp(MinY, 0, Buffer->Height);
	MaxY = Clamp(MaxY, 0, Buffer->Height);
    
	MinX = Clamp(MinX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MinY = Clamp(MinY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
	MaxX = Clamp(MaxX, (int)ClipRect.Min.x, (int)ClipRect.Max.x);
	MaxY = Clamp(MaxY, (int)ClipRect.Min.y, (int)ClipRect.Max.y);
    
	__m128 mmOne = _mm_set1_ps(1.0f);
	__m128 mmZero = _mm_set1_ps(0.0f);
	__m128 mm255 = _mm_set1_ps(255.0f);
	__m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
	__m128i mmFF = _mm_set1_epi32(0xFF);
	__m128i mmMaxX = _mm_set1_epi32(MaxX);
	__m128 mmMaxXF = _mm_cvtepi32_ps(mmMaxX);
    
	__m128 mmModColor_r = _mm_set1_ps(ModulationColor01.r);
	__m128 mmModColor_g = _mm_set1_ps(ModulationColor01.g);
	__m128 mmModColor_b = _mm_set1_ps(ModulationColor01.b);
	__m128 mmModColor_a = _mm_set1_ps(ModulationColor01.a);
    
	mmModColor_r = _mm_min_ps(_mm_max_ps(mmModColor_r, mmZero), mmOne);
	mmModColor_g = _mm_min_ps(_mm_max_ps(mmModColor_g, mmZero), mmOne);
	mmModColor_b = _mm_min_ps(_mm_max_ps(mmModColor_b, mmZero), mmOne);
	mmModColor_a = _mm_min_ps(_mm_max_ps(mmModColor_a, mmZero), mmOne);
    
	/*Premultiplying incoming color with it alpha*/
	mmModColor_r = _mm_mul_ps(_mm_mul_ps(mmModColor_r, mmModColor_a), mm255);
	mmModColor_g = _mm_mul_ps(_mm_mul_ps(mmModColor_g, mmModColor_a), mm255);
	mmModColor_b = _mm_mul_ps(_mm_mul_ps(mmModColor_b, mmModColor_a), mm255);
    
	__m128i mmDestWidth = _mm_set1_epi32(Buffer->Width);
	__m128 mmDestWidthF = _mm_cvtepi32_ps(mmDestWidth);
    
    
	for (int DestY = MinY; DestY < MaxY; DestY++) {
		for (int DestX = MinX; DestX < MaxX; DestX += 4) {
			__m128i mmDestX = _mm_setr_epi32(DestX, DestX + 1, DestX + 2, DestX + 3);
			__m128 mmDestXF = _mm_cvtepi32_ps(mmDestX);
            
			u32* OutDest = (u32*)((u8*)Buffer->Pixels + DestY * Buffer->Pitch + (DestX << 2));
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

void SoftwareRenderStackToOutput(render_stack* Stack, bmp_info* Buffer, rc2 ClipRect){
    u8* At = (u8*)Stack->Data.Base;
	u8* StackEnd = (u8*)Stack->Data.Base + Stack->Data.Used;
    
	while (At < StackEnd) {
        render_entry_header* Header = (render_entry_header*)At;
        
        At += sizeof(render_entry_header);
        
        switch(Header->Type){
            case RenderEntry_ClearColor:{
                render_entry_clear_color* Entry = (render_entry_clear_color*)At;
                
                RenderClearSSE(Buffer, Entry->ClearColor01, ClipRect);
            }break;
            
            case RenderEntry_Rect:{
                render_entry_rect* Entry = (render_entry_rect*)At;
                
#if 1
                RenderRectSSE(Buffer,
                              Entry->P,
                              Entry->Dim,
                              Entry->ModulationColor01,
                              ClipRect);
#else
                RenderRect(Buffer,
                           Entry->P,
                           Entry->Dim,
                           Entry->ModulationColor01,
                           ClipRect);
#endif
            }break;
            
            case RenderEntry_Bitmap:{
                render_entry_bitmap* Entry = (render_entry_bitmap*)At;
                
#if 1
                RenderBitmapSSE(Buffer, 
                                Entry->Bitmap,
                                Entry->TopLeftP,
                                Entry->PixelHeight,
                                Entry->ModulationColor01,
                                ClipRect);
#else
                RenderBitmap(Buffer, 
                             Entry->Bitmap,
                             Entry->TopLeftP,
                             Entry->PixelHeight,
                             Entry->ModulationColor01,
                             ClipRect);
#endif
            }break;
            
            case RenderEntry_Glyph:{
                
            }break;
        }
        
        At += Header->DataSize;
    }
}

struct render_queue_work{
    bmp_info* Buffer;
    rc2 ClipRect;
    render_stack* Stack;
};

PLATFORM_CALLBACK(RenderQueueWork){
    render_queue_work* Work = (render_queue_work*)Data;
    
    SoftwareRenderStackToOutput(
        Work->Stack,
        Work->Buffer,
        Work->ClipRect);
}

struct render_queue_rgba2bgra_work{
    bmp_info* Buffer;
    rc2 ClipRect;
};

PLATFORM_CALLBACK(RenderQueueRGBA2BGRAWork){
    render_queue_rgba2bgra_work* Work = (render_queue_rgba2bgra_work*)Data;
    
    RenderRGBA2BGRASSE(Work->Buffer, Work->ClipRect);
}

#define TILES_COUNT 32
void RenderMultithreaded(platform_job_queue* Queue, render_stack* Stack, bmp_info* Buffer) {
    
#if 1
	rc2 ClipRect;
	ClipRect.Min = V2(0, 0);
	ClipRect.Max = V2(Buffer->Width, Buffer->Height);
    
	SoftwareRenderStackToOutput(Stack, Buffer, ClipRect);
#else
    
#if 0
#define SIDE_TILES_COUNT 8
    
	render_queue_work Works[SIDE_TILES_COUNT * SIDE_TILES_COUNT];
    
	int SideTileWidth = Buffer->Width / SIDE_TILES_COUNT;
    SideTileWidth = (SideTileWidth + 15) & (~15);
	int SideTileHeight = Buffer->Height / SIDE_TILES_COUNT;
    
    for (int j = 0; j < SIDE_TILES_COUNT; j++) {
		for (int i = 0; i < SIDE_TILES_COUNT; i++) {
            rc2 Rect;
            
            Rect.Min = V2(SideTileWidth * i, SideTileHeight * j);
            Rect.Max = V2(SideTileWidth * (i + 1), SideTileHeight * (j + 1));
            
            if (j == SIDE_TILES_COUNT - 1) {
                Rect.Max.y = Buffer->Height;
            }
            
            if (i == SIDE_TILES_COUNT - 1) {
                Rect.Max.x = Buffer->Width;
            }
            
            render_queue_work* WorkData = &Works[j * SIDE_TILES_COUNT + i];
            WorkData->Buffer = Buffer;
            WorkData->Stack = Stack;
            WorkData->ClipRect = Rect;
            
            //if ((j & 1) == (i & 1)) {
            PlatformAddEntry(Queue, RenderQueueWork, WorkData);
            //}
		}
    }
#else
    render_queue_work Works[TILES_COUNT];
    
    int PreferableTileHeight = Buffer->Height / TILES_COUNT;
    if(PreferableTileHeight < 16){
        PreferableTileHeight = 16;
    }
    
    int CurrentH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 Rect;
        
        b32 ShouldExit = 0;
        int MaxHeight = CurrentH + PreferableTileHeight;
        if(MaxHeight > Buffer->Height){
            MaxHeight = Buffer->Height;
            ShouldExit = 1;
        }
        
        Rect.Min = V2(0.0f, CurrentH);
        Rect.Max = V2(Buffer->Width, MaxHeight);
        
        render_queue_work* WorkData = &Works[i];
        WorkData->Buffer = Buffer;
        WorkData->Stack = Stack;
        WorkData->ClipRect = Rect;
        
        //if (i & 1) {
        PlatformAddEntry(Queue, RenderQueueWork, WorkData);
        //}
        
        CurrentH += PreferableTileHeight;
        if(ShouldExit){
            break;
        }
    }
#endif
    
    PlatformWaitForCompletion(Queue);
#endif
}

void RenderMultithreadedRGBA2BGRA(platform_job_queue* Queue, bmp_info* Buffer) {
    
    render_queue_rgba2bgra_work Works[TILES_COUNT];
    
    int PreferableTileHeight = Buffer->Height / TILES_COUNT;
    if(PreferableTileHeight < 16){
        PreferableTileHeight = 16;
    }
    
    int CurrentH = 0; 
    for(int i = 0; i < TILES_COUNT; i++){
        rc2 Rect;
        
        b32 ShouldExit = 0;
        int MaxHeight = CurrentH + PreferableTileHeight;
        if(MaxHeight > Buffer->Height){
            MaxHeight = Buffer->Height;
            ShouldExit = 1;
        }
        
        Rect.Min = V2(0.0f, CurrentH);
        Rect.Max = V2(Buffer->Width, MaxHeight);
        
        render_queue_rgba2bgra_work* WorkData = &Works[i];
        WorkData->Buffer = Buffer;
        WorkData->ClipRect = Rect;
        
        //if (i & 1) {
        PlatformAddEntry(Queue, RenderQueueRGBA2BGRAWork, WorkData);
        //}
        
        CurrentH += PreferableTileHeight;
        if(ShouldExit){
            break;
        }
    }
    
    PlatformWaitForCompletion(Queue);
}
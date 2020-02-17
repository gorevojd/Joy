#include "joy_render_blur.h"


u32 Calcualte2DGaussianBoxComponentsCount(int Radius) {
	int Diameter = Radius + Radius + 1;
    
	u32 Result = Diameter * Diameter;
    
	return(Result);
}

void Normalize2DGaussianBox(float* Box, int Radius) {
	//NOTE(dima): Calculate sum of all elements
	float TempSum = 0.0f;
	int Diam = Radius + Radius + 1;
	for (int i = 0; i < Diam * Diam; i++) {
		TempSum += Box[i];
	}
    
	//NOTE(dima): Normalize elements
	float NormValueMul = 1.0f / TempSum;
    
	for (int i = 0; i < Diam * Diam; i++) {
		Box[i] *= NormValueMul;
	}
}

void Calculate2DGaussianBox(float* Box, int Radius) {
	int Diameter = Radius + Radius + 1;
    
	int Center = Radius;
    
	float Sigma = (float)Radius;
    
	float A = 1.0f / (2.0f * JOY_PI * Sigma * Sigma);
    
	float InExpDivisor = 2.0f * Sigma * Sigma;
    
	float TempSum = 0.0f;
    
	//NOTE(dima): Calculate elements
	for (int y = 0; y < Diameter; y++) {
		int PsiY = y - Center;
		for (int x = 0; x < Diameter; x++) {
			int PsiX = x - Center;
            
			float ValueToExp = -((PsiX * PsiX + PsiY * PsiY) / InExpDivisor);
            
			float Expon = Exp(ValueToExp);
            
			float ResultValue = A * Expon;
            
			Box[y * Diameter + x] = ResultValue;
			TempSum += ResultValue;
		}
	}
    
	//NOTE(dima): Normalize elements
	float NormValueMul = 1.0f / TempSum;
    
	for (int i = 0; i < Diameter * Diameter; i++) {
		Box[i] *= NormValueMul;
	}
}

static void BoxBlurApproximate(
bmp_info* To,
bmp_info* From,
int BlurRadius)
{
	int BlurDiam = 1 + BlurRadius + BlurRadius;
    
	for (int Y = 0; Y < From->Height; Y++) {
		for (int X = 0; X < From->Width; X++) {
            
			u32* TargetPixel = (u32*)((u8*)To->Pixels + Y * To->Pitch + X * 4);
            
			v4 VertSum = {};
			int VertSumCount = 0;
			for (int kY = Y - BlurRadius; kY <= Y + BlurRadius; kY++) {
				int targetY = Clamp(kY, 0, From->Height - 1);
                
				u32* ScanPixel = (u32*)((u8*)From->Pixels + targetY * From->Pitch + X * 4);
				v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                
				VertSum += UnpackedColor;
                
				VertSumCount++;
			}
            
            
			v4 HorzSum = {};
			int HorzSumCount = 0;
			for (int kX = X - BlurRadius; kX <= X + BlurRadius; kX++) {
				int targetX = Clamp(kX, 0, From->Width - 1);
                
				u32* ScanPixel = (u32*)((u8*)From->Pixels + Y * From->Pitch + targetX * 4);
				v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                
				HorzSum += UnpackedColor;
                
				HorzSumCount++;
			}
            
            
			VertSum = VertSum / (float)VertSumCount;
			HorzSum = HorzSum / (float)HorzSumCount;
            
			v4 TotalSum = (VertSum + HorzSum) * 0.5f;
            
			*TargetPixel = PackRGBA(TotalSum);
		}
	}
}

bmp_info BlurBitmapApproximateGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
void* TempBitmapMem,
int width, int height,
int BlurRadius)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	bmp_info TempBitmap = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		TempBitmapMem);
    
    
	/*
 var wIdeal = Math.sqrt((12 * sigma*sigma / n) + 1);  // Ideal averaging filter width
 var wl = Math.floor(wIdeal);  if (wl % 2 == 0) wl--;
 var wu = wl + 2;
 var mIdeal = (12 * sigma*sigma - n*wl*wl - 4 * n*wl - 3 * n) / (-4 * wl - 4);
 var m = Math.round(mIdeal);
 // var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );
 var sizes = [];  for (var i = 0; i<n; i++) sizes.push(i<m ? wl : wu);
 */
    
    
	float Boxes[3];
	int n = 3;
	float nf = 3.0f;
    
	float Sigma = (float)BlurRadius;
	float WIdeal = Sqrt((12.0f * Sigma * Sigma / nf) + 1.0f);
	float wlf = floorf(WIdeal);
	int wl = (float)(wlf + 0.5f);
	if (wl & 1 == 0) {
		wl--;
	}
	int wu = wl + 2;
    
	float mIdeal = (12.0f * Sigma * Sigma - nf * float(wl) * float(wl) - 4.0f * nf * float(wl) - 3.0f * nf) / (-4.0f * (float)wl - 4.0f);
	float mf = roundf(mIdeal);
	int m = float(mf + 0.5f);
    
	for (int i = 0; i < n; i++) {
		int ToSet = wu;
		if (i < m) {
			ToSet = wl;
		}
		Boxes[i] = ToSet;
	}
    
	BoxBlurApproximate(&Result, BitmapToBlur, (Boxes[0] - 1) / 2);
	BoxBlurApproximate(&TempBitmap, &Result, (Boxes[1] - 1) / 2);
	BoxBlurApproximate(&Result, &TempBitmap, (Boxes[2] - 1) / 2);
    
	return(Result);
}

bmp_info BlurBitmapExactGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
int width, int height,
int BlurRadius,
float* GaussianBox)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	int BlurDiam = 1 + BlurRadius + BlurRadius;
    
	bmp_info* From = BitmapToBlur;
	bmp_info* To = &Result;
    
	for (int Y = 0; Y < From->Height; Y++) {
		for (int X = 0; X < From->Width; X++) {
            
			u32* TargetPixel = (u32*)((u8*)To->Pixels + Y * To->Pitch + X * 4);
            
			v4 SumColor = {};
			for (int kY = Y - BlurRadius; kY <= Y + BlurRadius; kY++) {
				int targetY = Clamp(kY, 0, From->Height - 1);
				int inboxY = kY - (Y - BlurRadius);
				for (int kX = X - BlurRadius; kX <= X + BlurRadius; kX++) {
					int targetX = Clamp(kX, 0, From->Width - 1);
					int inboxX = kX - (X - BlurRadius);
                    
					u32* ScanPixel = (u32*)((u8*)From->Pixels + targetY * From->Pitch + targetX * 4);
                    
					v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                    
					SumColor += UnpackedColor * GaussianBox[inboxY * BlurDiam + inboxX];
				}
			}
            
			*TargetPixel = PackRGBA(SumColor);
		}
	}
    
	return(Result);
}
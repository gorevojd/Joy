#ifndef JOY_RENDER_BLUR_H_INCLUDED
#define JOY_RENDER_BLUR_H_INCLUDED

#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"

/*

int BlurRadius = 2;

 intcomponent_count = Calcualte2DGaussianBoxComponentsCount(BlurRadius);
  float* GaussianBox = (float*)allocate_array_of_count(component_count);
  Calculate2DGaussianBox(GaussianBox, BlurRadius);
  
*/

u32 Calcualte2DGaussianBoxComponentsCount(int Radius);
void Normalize2DGaussianBox(float* Box, int Radius);
void Calculate2DGaussianBox(float* Box, int Radius);

bmp_info BlurBitmapApproximateGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
void* TempBitmapMem,
int Width, int Height,
int BlurRadius);

bmp_info BlurBitmapExactGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
int Width, int Height,
int BlurRadius,
float* GaussianBox);


#endif
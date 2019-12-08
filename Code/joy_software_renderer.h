#ifndef JOY_SOFTWARE_RENDERER_H
#define JOY_SOFTWARE_RENDERER_H

#include "joy_math.h"
#include "joy_assets.h"
#include "joy_render_stack.h"
#include "joy_platform.h"

void RenderClear(bmp_info* Buffer, v3 Color, rc2 ClipRect);
void RenderClearSSE(bmp_info* Buffer, v3 Color, rc2 ClipRect);

/*
NOTE(dima): For this function the following conditions must be true:
 StartX + RenderWhat->Width <= RenderTo->Width
 StartY + RenderWhat->Height <= RenderTo->Height
 
*/
void RenderOneBitmapIntoAnother(
bmp_info* RenderTo, 
bmp_info* RenderWhat,
int StartX,
int StartY,
v4 ModulationColor);

void RenderRGBA2BGRA(
bmp_info* Buffer, 
rc2 ClipRect);

void RenderRGBA2BGRASSE(
bmp_info* Buffer, 
rc2 ClipRect);

void RenderBitmap(
bmp_info* Buffer,
bmp_info* Bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 ModulationColor01, 
rc2 ClipRect);
void RenderBitmapSSE(
bmp_info* Buffer,
bmp_info* Bitmap,
v2 P,
float TargetBitmapPixelHeight,
v4 ModulationColor01,
rc2 ClipRect);

void RenderRect(
bmp_info* Buffer,
v2 P,
v2 Dim,
v4 ModulationColor01, 
rc2 ClipRect);
void RenderRectSSE(
bmp_info* Buffer,
v2 P,
v2 Dim,
v4 ModulationColor01,
rc2 ClipRect);

void RenderMultithreaded(platform_job_queue* Queue, render_stack* Stack, bmp_info* Buffer);
void RenderMultithreadedRGBA2BGRA(platform_job_queue* Queue, bmp_info* Buffer);

#endif
#ifndef JOY_SOFTWARE_RENDERER_FUNCTIONS_H
#define JOY_SOFTWARE_RENDERER_FUNCTIONS_H

#include "joy_math.h"
#include "joy_render_stack.h"
#include "joy_platform.h"


void RenderClear(bmp_info* buf, v3 color, rc2 clipRect);
#if defined(JOY_AVX)
void RenderClearSSE(bmp_info* buf, v3 color, rc2 clipRect);
#endif

/*
NOTE(dima): For this function the following conditions must be true:
 StartX + RenderWhat->Width <= RenderTo->Width
 StartY + RenderWhat->Height <= RenderTo->Height
 
*/
void RenderOneBitmapIntoAnother(
                                bmp_info* renderTo, 
                                bmp_info* renderWhat,
                                int startX,
                                int startY,
                                v4 modulationColor);

void RenderRGBA2BGRA(bmp_info* buf, rc2 clipRect);
#if defined(JOY_AVX)
void RenderRGBA2BGRASSE(bmp_info* buf, rc2 clipRect);
#endif

void RenderGradientHorz(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);
#if defined(JOY_AVX)
void RenderGradientHorzSSE(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);
#endif

void RenderGradientVert(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);
#if defined(JOY_AVX)
void RenderGradientVertSSE(bmp_info* buf, rc2 rect, v3 color1, v3 color2, rc2 clipRect);
#endif

void RenderBitmap(
                  bmp_info* buf,
                  bmp_info* bmp,
                  v2 p,
                  float targetBitmapPixelHeight,
                  v4 modulationColor01, 
                  rc2 clipRect);
#if defined(JOY_AVX)
void RenderBitmapSSE(
                     bmp_info* buf,
                     bmp_info* bmp,
                     v2 p,
                     float targetBitmapPixelHeight,
                     v4 modulationColor01,
                     rc2 clipRect);
#endif

void RenderRect(
                bmp_info* buf,
                v2 p,
                v2 dim,
                v4 modulationColor01, 
                rc2 clipRect);
#if defined(JOY_AVX)
void RenderRectSSE(
                   bmp_info* buf,
                   v2 p,
                   v2 dim,
                   v4 modulationColor01,
                   rc2 clipRect);
#endif

#endif
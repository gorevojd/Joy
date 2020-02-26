#ifndef JOY_SOFTWARE_RENDERER_FUNCTIONS_H
#define JOY_SOFTWARE_RENDERER_FUNCTIONS_H

#include "joy_math.h"
#include "joy_assets.h"
#include "joy_render_stack.h"
#include "joy_platform.h"


void RenderClear(bmp_info* buf, v3 color, rc2 clipRect);
void RenderClearSSE(bmp_info* buf, v3 color, rc2 clipRect);

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
void RenderRGBA2BGRASSE(bmp_info* buf, rc2 clipRect);

void RenderGradientHorz(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);
void RenderGradientHorzSSE(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);

void RenderGradientVert(bmp_info* buf, rc2 rect, v3 color1, v3 Color2, rc2 clipRect);
void RenderGradientVertSSE(bmp_info* buf, rc2 rect, v3 color1, v3 color2, rc2 clipRect);

void RenderBitmap(
bmp_info* buf,
bmp_info* bmp,
v2 p,
float targetBitmapPixelHeight,
v4 modulationColor01, 
rc2 clipRect);
void RenderBitmapSSE(
bmp_info* buf,
bmp_info* bmp,
v2 p,
float targetBitmapPixelHeight,
v4 modulationColor01,
rc2 clipRect);

void RenderRect(
bmp_info* buf,
v2 p,
v2 dim,
v4 modulationColor01, 
rc2 clipRect);
void RenderRectSSE(
bmp_info* buf,
v2 p,
v2 dim,
v4 modulationColor01,
rc2 clipRect);


#endif
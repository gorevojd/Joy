#ifndef JOY_SOFTWARE_RENDERER_H
#define JOY_SOFTWARE_RENDERER_H

#include "joy_math.h"
#include "joy_assets.h"
#include "joy_render_stack.h"
#include "joy_platform.h"

void RenderClear(Bmp_Info* buf, v3 color, rc2 clipRect);
void RenderClearSSE(Bmp_Info* buf, v3 color, rc2 clipRect);

/*
NOTE(dima): For this function the following conditions must be true:
 StartX + RenderWhat->Width <= RenderTo->Width
 StartY + RenderWhat->Height <= RenderTo->Height
 
*/
void RenderOneBitmapIntoAnother(
Bmp_Info* renderTo, 
Bmp_Info* renderWhat,
int startX,
int startY,
v4 modulationColor);

void RenderRGBA2BGRA(Bmp_Info* buf, rc2 clipRect);
void RenderRGBA2BGRASSE(Bmp_Info* buf, rc2 clipRect);

void RenderGradientHorz(Bmp_Info* buf, v3 color, rc2 clipRect);
void RenderGradientHorzSSE(Bmp_Info* buf, v3 color, rc2 clipRect);

void RenderGradientVert(Bmp_Info* buf, v3 color, rc2 clipRect);
void RenderGradientVertSSE(Bmp_Info* buf, v3 color, rc2 clipRect);


void RenderBitmap(
Bmp_Info* buf,
Bmp_Info* bmp,
v2 p,
float targetBitmapPixelHeight,
v4 modulationColor01, 
rc2 clipRect);
void RenderBitmapSSE(
Bmp_Info* buf,
Bmp_Info* bmp,
v2 p,
float targetBitmapPixelHeight,
v4 modulationColor01,
rc2 clipRect);

void RenderRect(
Bmp_Info* buf,
v2 p,
v2 dim,
v4 modulationColor01, 
rc2 clipRect);
void RenderRectSSE(
Bmp_Info* buf,
v2 p,
v2 dim,
v4 modulationColor01,
rc2 clipRect);

void RenderMultithreaded(Platform_Job_Queue* queue, Render_Stack* stack, Bmp_Info* buffer);
void RenderMultithreadedRGBA2BGRA(Platform_Job_Queue* queue, Bmp_Info* buffer);

#endif
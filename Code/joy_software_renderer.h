#ifndef JOY_SOFTWARE_RENDERER_H
#define JOY_SOFTWARE_RENDERER_H

#include "joy_software_renderer_functions.h"

void RenderMultithreaded(platform_job_queue* queue, render_stack* stack, bmp_info* buffer);
void RenderMultithreadedRGBA2BGRA(platform_job_queue* queue, bmp_info* buffer);

#endif
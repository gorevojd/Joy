#ifndef JOY_SOFTWARE_RENDERER_H
#define JOY_SOFTWARE_RENDERER_H

#include "joy_software_renderer_functions.h"

void RenderMultithreaded(platform_job_queue* queue, render_state* stack, render_primitive_bitmap* buffer);
void RenderMultithreadedRGBA2BGRA(platform_job_queue* queue, render_primitive_bitmap* buffer);

#endif
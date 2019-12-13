#ifndef WIN32_JOY_H
#define WIN32_JOY_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>

#include "joy_types.h"
#include "joy_math.h"
#include "joy_defines.h"
#include "joy_memory.h"

#include "joy_input.h"
#include "joy_assets.h"
#include "joy_software_renderer.h"
#include "joy_platform.h"
#include "joy_gui.h"

#include "joy_opengl_defs.h"
#include "win32_opengl.h"

struct win32_state{
	HWND Window;
    int WindowWidth;
    int WindowHeight;
    
    WINDOWPLACEMENT WindowPlacement;
    
    HGLRC RenderContext;
    
    bmp_info Bitmap;
    BITMAPINFO BMI;
    
    LARGE_INTEGER PerformanceFreqLI;
    float OneOverPerformanceFreq;
};

#endif
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

#define USE_VULKAN 0
#define USE_OPENGL 1

#if USE_VULKAN
#include "win32_vulkan.h"
#endif

#if USE_OPENGL
#include "win32_opengl.h"
#endif

struct win32_state{
	HWND Window;
    int WindowWidth;
    int WindowHeight;
    
#if USE_VULKAN
	vulkan_state Vulkan;
#endif
    
#if USE_OPENGL
    win32_opengl Win32GL;
#endif
    
    WINDOWPLACEMENT WindowPlacement;
    
    bmp_info Bitmap;
    BITMAPINFO BMI;
    
    LARGE_INTEGER PerformanceFreqLI;
    float OneOverPerformanceFreq;
};

#endif
#ifndef WIN32_JOY_H
#define WIN32_JOY_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <dsound.h>
#include <xinput.h>

#include "joy_types.h"
#include "joy_math.h"
#include "joy_defines.h"
#include "joy_memory.h"

#include "joy_platform.h"

#define JOY_USE_OPENGL 1
#define JOY_USE_DIRECTX 0

#if JOY_USE_OPENGL
#define JOY_OPENGL_DEFS_DECLARATION
#include "joy_opengl_defs.h"

#include "joy_opengl.h"
#endif

#ifndef WGL_ARB_multisample
#define WGL_ARB_multisample 1

#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

#endif /* WGL_ARB_multisample */

#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context 1
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096
#endif /* WGL_ARB_create_context */

#ifndef WGL_ARB_create_context_profile
#define WGL_ARB_create_context_profile 1
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif /* WGL_ARB_create_context_profile */

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format 1
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#endif /* WGL_ARB_pixel_format */

typedef BOOL (WINAPI * PFN_wglChoosePixelFormatARB) (HDC hdc, 
                                                     const int* piAttribIList, 
                                                     const FLOAT *pfAttribFList, 
                                                     UINT nMaxFormats, 
                                                     int *piFormats, 
                                                     UINT *nNumFormats);

typedef HGLRC (WINAPI * PFN_wglCreateContextAttribsARB) (HDC hDC, 
                                                         HGLRC hShareContext, 
                                                         const int* attribList);

typedef const char* (WINAPI * PFN_wglGetExtensionStringARB) (HDC hdc);
typedef const char* (WINAPI * PFN_wglGetExtensionStringEXT) (void);

typedef int (WINAPI * PFN_wglGetSwapIntervalEXT) (void);
typedef BOOL (WINAPI * PFN_wglSwapIntervalEXT) (int interval);

#define GETGLFUN(fun) fun = (PFN_##fun*)wglGetProcAddress(#fun)
#define WGLGETFUN(fun) fun = (PFN_##fun)wglGetProcAddress(#fun);

#define WGLFUN(fun) PFN_##fun fun;
WGLFUN(wglChoosePixelFormatARB);
WGLFUN(wglCreateContextAttribsARB);
WGLFUN(wglGetExtensionStringARB);
WGLFUN(wglGetExtensionStringEXT);
WGLFUN(wglGetSwapIntervalEXT);
WGLFUN(wglSwapIntervalEXT);

// NOTE(Dima): This is defined here for joy_dirx files.
PLATFORM_SHOW_ERROR(Win32ShowError);
PLATFORM_DEBUG_OUTPUT_STRING(Win32DebugOutputString);

struct DSound_State{
    b32 secondaryBufferCreated;
    b32 captureBufferCreated;
    
    WAVEFORMATEX waveFormat;
    
    // NOTE(Dima): DirectSound stuff
    LPDIRECTSOUND8 dSound;
    LPDIRECTSOUNDBUFFER primBuf;
    LPDIRECTSOUNDBUFFER secBuf;
    DSBUFFERDESC secBufDesc;
    
    // NOTE(Dima): DirectSoundCapture stuff
    LPDIRECTSOUNDCAPTURE8 dCapture;
    LPDIRECTSOUNDCAPTUREBUFFER dCaptureBuf;
    DSCBUFFERDESC dCaptureDesc;
};

struct Win_Memory_Region{
    mem_block_entry Block;
    
    // NOTE(Dima): Total commited size
    mi TotalCommittedSize;
    
    Win_Memory_Region* Next;
    Win_Memory_Region* Prev;
};

struct win_critical_section_slot{
    CRITICAL_SECTION Section;
    b32 InUse;
};

struct win_state{
	HWND window;
    int InitWindowWidth;
    int InitWindowHeight;
    int WindowWidth;
    int WindowHeight;
    b32 ToggledFullscreen;
    
    std::vector<std::string> LoadedStringsHolder;
    b32 InListFilesBlock;
    
    b32 OpenFilesNextFound;
    b32 InOpenFilesBlock;
    WIN32_FIND_DATAA OpenFilesFindData;
    HANDLE OpenFilesFindHandle;
    char OpenFilesDirectory[MAX_PATH];
    
    WINDOWPLACEMENT windowPlacement;
    XINPUT_STATE ControllerStates[XUSER_MAX_COUNT];
    
#define MAX_CRITICAL_SECTIONS_COUNT 1024
    win_critical_section_slot CriticalSections[MAX_CRITICAL_SECTIONS_COUNT];
    u16 NotUsedCriticalSectionIndices[MAX_CRITICAL_SECTIONS_COUNT];
    int NotUsedCriticalSectionIndicesCount;
    
    HDC glDC;
    HGLRC renderCtx;
    
    bmp_info bitmap;
    BITMAPINFO bmi;
    
    ticket_mutex memoryMutex;
    Win_Memory_Region memorySentinel;
    
    LARGE_INTEGER PerformanceFreqLI;
    double OneOverPerformanceFreq;
};

HGLRC Win32InitOpenGL(HDC realDC);
void Win32FreeOpenGL(HGLRC renderContext);

#endif
#ifndef JOY_DIRX_H
#define JOY_DIRX_H

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <Windows.h>

#include "joy_platform.h"
#include "joy_types.h"

// include the Direct3D Library file
//#pragma comment(linker, "/LIBPATH:C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "winmm.lib")

struct DirX_State{
    IDXGISwapChain* swapChain;
    ID3D11Device* dev;
    ID3D11DeviceContext* devCtx;
    ID3D11RenderTargetView* backBuffer;
};

extern PLATFORM_SHOW_ERROR(Win32ShowError);
extern PLATFORM_DEBUG_OUTPUT_STRING(Win32DebugOutputString);

// NOTE(Dima): direct sound stuff
void DSoundInit(DSound_State* dsState, HWND hwnd);
void DSoundFree(DSound_State* ds);

// NOTE(Dima): directX stuff
void DirXSetViewport(DirX_State* dirX, int width,
                     int height);
void DirXInit(DirX_State* dirX, HWND hwnd, int width, int height);
void DirXFree(DirX_State* dirX);

#endif
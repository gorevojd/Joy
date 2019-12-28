#include "joy_dirx.h"

void DirXSetViewport(DirX_State* dirX, int width,
                     int height)
{
    // NOTE(Dima): Setting the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = width;
    viewport.Height = height;
    
    dirX->devCtx->RSSetViewports(1, &viewport);
}

void DirXInit(DirX_State* dirX, HWND hwnd, int width, int height)
{
    // NOTE(Dima): Swap chain info
    DXGI_SWAP_CHAIN_DESC scd;
    
    // NOTE(Dima): clearing structure
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    
    // NOTE(Dima): Filling the info
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    
    // NOTE(Dima): Creating swap chain and device
    D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        NULL,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &dirX->swapChain,
        &dirX->dev,
        NULL,
        &dirX->devCtx);
    
    // NOTE(Dima): Get the address of the backbuffer
    ID3D11Texture2D* pBackBuffer;
    dirX->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    
    // NOTE(Dima): Use back buffer to create render target
    dirX->dev->CreateRenderTargetView(pBackBuffer,
                                      NULL,
                                      &dirX->backBuffer);
    pBackBuffer->Release();
    
    // NOTE(Dima): Set the render target as the back buffer
    dirX->devCtx->OMSetRenderTargets(1, &dirX->backBuffer, NULL);
}

void DirXFree(DirX_State* dirX){
    dirX->swapChain->Release();
    dirX->backBuffer->Release();
    dirX->dev->Release();
    dirX->devCtx->Release();
}


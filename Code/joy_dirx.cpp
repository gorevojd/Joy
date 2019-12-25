#include "joy_dirx.h"

/*
Note that Direct Sound does use two different kinds of buffers which are primary and secondary
 buffers. The primary buffer is the main sound memory buffer on your default sound card, USB
 headset, and so forth. Secondary buffers are buffers you create in memory and load your sounds
 into. When you play a secondary buffer the Direct Sound API takes care of mixing that sound
 into the primary buffer which then plays the sound. If you play multiple secondary buffers at
 the same time it will mix them together and play them in the primary buffer. Also note that all
 buffers are circular so you can set them to repeat indefinitely.
*/
void DSoundInit(DSound_State* dsState, HWND hwnd){
    dsState->secondaryBufferCreated = 0;
    
    if(SUCCEEDED(DirectSoundCreate8(0, &dsState->dSound, 0))){
        // NOTE(Dima): Setting cooperative level
        if(SUCCEEDED(dsState->dSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))){
            DSBUFFERDESC primBufDesc;
            primBufDesc.dwSize = sizeof(DSBUFFERDESC);
            primBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
            primBufDesc.dwBufferBytes = 0;
            primBufDesc.lpwfxFormat = 0;
            primBufDesc.guid3DAlgorithm = GUID_NULL;
            primBufDesc.dwReserved = 0;
            
            // NOTE(Dima): Creating primary buffer
            HRESULT primBufRes = dsState->dSound->CreateSoundBuffer(
                &primBufDesc, 
                &dsState->primBuf, 
                0);
            
            WAVEFORMATEX* pwf = &dsState->waveFormat;
            
            if(SUCCEEDED(primBufRes)){
                pwf->wFormatTag = WAVE_FORMAT_PCM;
                pwf->nSamplesPerSec = 44100;
                pwf->wBitsPerSample = 16;
                pwf->nChannels = 2;
                pwf->nBlockAlign = (pwf->wBitsPerSample / 8) * pwf->nChannels;
                pwf->nAvgBytesPerSec = pwf->nSamplesPerSec * pwf->nBlockAlign;
                pwf->cbSize = 0;
                
                // NOTE(Dima): setting wave format
                if(SUCCEEDED(dsState->primBuf->SetFormat(pwf))){
                    DSBUFFERDESC secBufDesc = {};
                    secBufDesc.dwSize = sizeof(DSBUFFERDESC);
                    secBufDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
                    secBufDesc.dwBufferBytes = pwf->nAvgBytesPerSec * 2; // 2 sesond lenght buffer
                    secBufDesc.lpwfxFormat = pwf;
                    secBufDesc.dwReserved = 0;
                    secBufDesc.guid3DAlgorithm = GUID_NULL;
                    
                    // NOTE(Dima): Creating secondary buffer
                    if(SUCCEEDED(dsState->dSound->CreateSoundBuffer(&secBufDesc, &dsState->secBuf, 0)))
                    {
                        // NOTE(Dima): EVERYTHING SUCCEEDED
                        dsState->secBufDesc = secBufDesc;
                        dsState->secondaryBufferCreated = 1;
                    }
                    else{
                        Win32ShowError(
                            PlatformError_Error, "DirectSound can not create secondary buffer");
                    }
                }
                else{
                    Win32ShowError(
                        PlatformError_Error, 
                        "Can not set DirectSound format");
                }
            }
            else{
                Win32ShowError(
                    PlatformError_Error, 
                    "Can not create DirectSound primary buffer");
            }
        }
        else{
            Win32ShowError(
                PlatformError_Error, 
                "DirectSound can not set cooperative level");
        }
    }
    else{
        Win32ShowError(
            PlatformError_Error, 
            "Can not create DirectSound object");
    }
}

void DSoundFree(DSound_State* ds){
    if(ds->secBuf){
        ds->secBuf->Release();
        ds->secBuf = 0;
    }
    
    if(ds->primBuf){
        ds->primBuf->Release();
        ds->primBuf = 0;
    }
    
    if(ds->dSound){
        ds->dSound->Release();
        ds->dSound = 0;
    }
}

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


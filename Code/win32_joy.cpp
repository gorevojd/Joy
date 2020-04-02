#include "win32_joy.h"

#if JOY_USE_DIRECTX
#include "joy_dirx.h"
#endif

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

/*
dima privet , kak dela? i tebia lybly
*/

GLOBAL_VARIABLE mem_region gMem = {};
GLOBAL_VARIABLE win_state GlobalWin32;
GLOBAL_VARIABLE b32 GlobalRunning;
GLOBAL_VARIABLE DSound_State GlobalDirectSound;
GLOBAL_VARIABLE mem_region GlobalMem;
GLOBAL_VARIABLE game_state* GlobalGame;

GLOBAL_VARIABLE f64 Time = 0.0f;
GLOBAL_VARIABLE f64 DeltaTime = 0.0f;

#if DEBUG_ENABLED
debug_record_table* GlobalRecordTable;
#endif

#if JOY_USE_OPENGL
GLOBAL_VARIABLE gl_state GlobalGL;
#endif
#if JOY_USE_DIRECTX
GLOBAL_VARIABLE DirX_State GlobalDirX;
#endif

platform_api Platform;


BOOL CALLBACK DirectSoundEnumerateCallback( 
LPGUID lpGuid, 
LPCSTR lpcstrDescription, 
LPCSTR lpcstrModule, 
LPVOID lpContext )
{
    Win32DebugOutputString((char*)lpcstrDescription);
    
    return(TRUE);
}


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
    dsState->captureBufferCreated = 0;
    
    //DirectSoundCaptureEnumerate((LPDSENUMCALLBACK)DirectSoundEnumerateCallback, 0);
    //DirectSoundEnumerate((LPDSENUMCALLBACK)DirectSoundEnumerateCallback, 0);
    
    WAVEFORMATEX* pwf = &dsState->waveFormat;
    pwf->wFormatTag = WAVE_FORMAT_PCM;
    pwf->nSamplesPerSec = 44100;
    pwf->wBitsPerSample = 16;
    pwf->nChannels = 2;
    pwf->nBlockAlign = (pwf->wBitsPerSample / 8) * pwf->nChannels;
    pwf->nAvgBytesPerSec = pwf->nSamplesPerSec * pwf->nBlockAlign;
    pwf->cbSize = 0;
    
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
            
            if(SUCCEEDED(primBufRes)){
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
    
    if(SUCCEEDED(DirectSoundCaptureCreate8(0, &dsState->dCapture, 0))){
        DSCBUFFERDESC dCaptureDesc = {};
		dCaptureDesc.dwSize = sizeof(DSCBUFFERDESC);
		dCaptureDesc.dwFlags = 0;
		dCaptureDesc.dwBufferBytes = pwf->nAvgBytesPerSec * 4;
		dCaptureDesc.dwReserved = 0;
		dCaptureDesc.lpwfxFormat = pwf;
		dCaptureDesc.dwFXCount = 0;
		dCaptureDesc.lpDSCFXDesc = NULL;
        
        if (SUCCEEDED(dsState->dCapture->CreateCaptureBuffer(
            &dCaptureDesc,
            &dsState->dCaptureBuf, 0)))
		{
            dsState->dCaptureDesc = dCaptureDesc;
            dsState->captureBufferCreated = 1;
        }
        else{
            Win32ShowError(
                PlatformError_Error, 
                "Can not create DirectSoundCapture buffer");
        }
    }
    else{
        Win32ShowError(
            PlatformError_Error, 
            "Can not create DirectSoundCapture object");
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

INTERNAL_FUNCTION void
Win32ClearSoundBuffer(DSound_State* ds)
{
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;
    
    HRESULT lockResult = ds->secBuf->Lock(
        0, 
        ds->secBufDesc.dwBufferBytes,
        &region1,
        &region1Size,
        &region2,
        &region2Size,
        DSBLOCK_ENTIREBUFFER);
    
    if(SUCCEEDED(lockResult)){
        int r1NBlocks = region1Size / (ds->waveFormat.nBlockAlign);
        int r2NBlocks = region2Size / (ds->waveFormat.nBlockAlign);
        
        
        i16* sampleAt = (i16*)region1;
        for(int bIndex = 0; bIndex < r1NBlocks;
            bIndex++)
        {
            *sampleAt++ = 0;
            *sampleAt++ = 0;
        }
        
        sampleAt = (i16*)region2;
        for(int bIndex = 0;
            bIndex < r2NBlocks;
            bIndex++)
        {
            *sampleAt++ = 0;
            *sampleAt++ = 0;
        }
        
        HRESULT unlockResult = ds->secBuf->Unlock(region1, region1Size,
                                                  region2, region2Size);
    }
}

INTERNAL_FUNCTION void 
Win32FillSoundBufferWithSound(DSound_State* ds, sound_info* sound)
{
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;
    
    HRESULT lockResult = ds->secBuf->Lock(
        0, 
        ds->secBufDesc.dwBufferBytes,
        &region1,
        &region1Size,
        &region2,
        &region2Size,
        DSBLOCK_ENTIREBUFFER);
    
    if(SUCCEEDED(lockResult)){
        int r1NBlocks = region1Size / (ds->waveFormat.nBlockAlign);
        int r2NBlocks = region2Size / (ds->waveFormat.nBlockAlign);
        
        i16* sampleAt = (i16*)region1;
        i16* sampleFromLeft = (i16*)sound->Samples[0];
        i16* sampleFromRight = (i16*)sound->Samples[1];
        
        for(int bIndex = 0; 
            bIndex < r1NBlocks;
            bIndex++)
        {
            *sampleAt++ = *sampleFromLeft++;
            *sampleAt++ = *sampleFromRight++;
        }
        
        sampleAt = (i16*)region2;
        for(int bIndex = 0;
            bIndex < r2NBlocks;
            bIndex++)
        {
            *sampleAt++ = *sampleFromLeft++;
            *sampleAt++ = *sampleFromRight++;
        }
        
        HRESULT unlockResult = ds->secBuf->Unlock(region1, region1Size,
                                                  region2, region2Size);
    }
}

INTERNAL_FUNCTION void 
Win32PlayDirectSoundBuffer(DSound_State* ds){
    ds->secBuf->Play(0, 0, DSBPLAY_LOOPING);
}

INTERNAL_FUNCTION void
Win32StopDirectSoundBuffer(DSound_State* ds){
    
}

LRESULT CALLBACK
TmpOpenGLWndProc(
HWND Window,
UINT Message,
WPARAM WParam,
LPARAM LParam)
{
    return DefWindowProc(Window, Message, WParam, LParam);
}

// NOTE(Dima): This function used to load wgl extensions
INTERNAL_FUNCTION void Win32LoadOpenglExtensions(){
    WNDCLASSA WndClass = {};
    WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WndClass.lpfnWndProc = TmpOpenGLWndProc;
    WndClass.hInstance = GetModuleHandle(0);
    WndClass.lpszClassName = "TmpClassName";
    
    RegisterClassA(&WndClass);
    
    HWND tmpWND = CreateWindowExA(
        0,
        WndClass.lpszClassName,
        "TmpWindow",
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0, 0,
        WndClass.hInstance,
        0);
    
    HDC tmpDC = GetDC(tmpWND);
    
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(pfd),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };
    int pFormat = ChoosePixelFormat(tmpDC, &pfd);
    DescribePixelFormat(tmpDC, pFormat, sizeof(pfd), &pfd);
    BOOL SetPixelFormatResult = SetPixelFormat(tmpDC, pFormat, &pfd);
    Assert(SetPixelFormatResult);
    
    HGLRC TmpRenderCtx = wglCreateContext(tmpDC);
    BOOL MakeCurrentResult = wglMakeCurrent(tmpDC, TmpRenderCtx);
    Assert(MakeCurrentResult);
    
    WGLGETFUN(wglChoosePixelFormatARB);
    WGLGETFUN(wglCreateContextAttribsARB);
    WGLGETFUN(wglGetExtensionStringARB);
    WGLGETFUN(wglGetExtensionStringEXT);
    WGLGETFUN(wglGetSwapIntervalEXT);
    WGLGETFUN(wglSwapIntervalEXT);
    
    wglMakeCurrent(tmpDC, 0);
    wglDeleteContext(TmpRenderCtx);
    ReleaseDC(tmpWND, tmpDC);
    DestroyWindow(tmpWND);
}

INTERNAL_FUNCTION void Win32GetOpenglFunctions(){
    GETGLFUN(glDrawRangeElements);
    GETGLFUN(glTexImage3D);
    GETGLFUN(glTexSubImage3D);
    GETGLFUN(glCopyTexSubImage3D);
    GETGLFUN(glActiveTexture);
    GETGLFUN(glSampleCoverage);
    GETGLFUN(glCompressedTexImage3D);
    GETGLFUN(glCompressedTexImage2D);
    GETGLFUN(glCompressedTexImage1D);
    GETGLFUN(glCompressedTexSubImage3D);
    GETGLFUN(glCompressedTexSubImage2D);
    GETGLFUN(glCompressedTexSubImage1D);
    GETGLFUN(glGetCompressedTexImage);
    GETGLFUN(glClientActiveTexture);
    GETGLFUN(glMultiTexCoord1d);
    GETGLFUN(glMultiTexCoord1dv);
    GETGLFUN(glMultiTexCoord1f);
    GETGLFUN(glMultiTexCoord1fv);
    GETGLFUN(glMultiTexCoord1i);
    GETGLFUN(glMultiTexCoord1iv);
    GETGLFUN(glMultiTexCoord1s);
    GETGLFUN(glMultiTexCoord1sv);
    GETGLFUN(glMultiTexCoord2d);
    GETGLFUN(glMultiTexCoord2dv);
    GETGLFUN(glMultiTexCoord2f);
    GETGLFUN(glMultiTexCoord2fv);
    GETGLFUN(glMultiTexCoord2i);
    GETGLFUN(glMultiTexCoord2iv);
    GETGLFUN(glMultiTexCoord2s);
    GETGLFUN(glMultiTexCoord2sv);
    GETGLFUN(glMultiTexCoord3d);
    GETGLFUN(glMultiTexCoord3dv);
    GETGLFUN(glMultiTexCoord3f);
    GETGLFUN(glMultiTexCoord3fv);
    GETGLFUN(glMultiTexCoord3i);
    GETGLFUN(glMultiTexCoord3iv);
    GETGLFUN(glMultiTexCoord3s);
    GETGLFUN(glMultiTexCoord3sv);
    GETGLFUN(glMultiTexCoord4d);
    GETGLFUN(glMultiTexCoord4dv);
    GETGLFUN(glMultiTexCoord4f);
    GETGLFUN(glMultiTexCoord4fv);
    GETGLFUN(glMultiTexCoord4i);
    GETGLFUN(glMultiTexCoord4iv);
    GETGLFUN(glMultiTexCoord4s);
    GETGLFUN(glMultiTexCoord4sv);
    GETGLFUN(glLoadTransposeMatrixf);
    GETGLFUN(glLoadTransposeMatrixd);
    GETGLFUN(glMultTransposeMatrixf);
    GETGLFUN(glMultTransposeMatrixd);
    GETGLFUN(glBlendFuncSeparate);
    GETGLFUN(glMultiDrawArrays);
    GETGLFUN(glMultiDrawElements);
    GETGLFUN(glPointParameterf);
    GETGLFUN(glPointParameterfv);
    GETGLFUN(glPointParameteri);
    GETGLFUN(glPointParameteriv);
    GETGLFUN(glFogCoordf);
    GETGLFUN(glFogCoordfv);
    GETGLFUN(glFogCoordd);
    GETGLFUN(glFogCoorddv);
    GETGLFUN(glFogCoordPointer);
    GETGLFUN(glSecondaryColor3b);
    GETGLFUN(glSecondaryColor3bv);
    GETGLFUN(glSecondaryColor3d);
    GETGLFUN(glSecondaryColor3dv);
    GETGLFUN(glSecondaryColor3f);
    GETGLFUN(glSecondaryColor3fv);
    GETGLFUN(glSecondaryColor3i);
    GETGLFUN(glSecondaryColor3iv);
    GETGLFUN(glSecondaryColor3s);
    GETGLFUN(glSecondaryColor3sv);
    GETGLFUN(glSecondaryColor3ub);
    GETGLFUN(glSecondaryColor3ubv);
    GETGLFUN(glSecondaryColor3ui);
    GETGLFUN(glSecondaryColor3uiv);
    GETGLFUN(glSecondaryColor3us);
    GETGLFUN(glSecondaryColor3usv);
    GETGLFUN(glSecondaryColorPointer);
    GETGLFUN(glWindowPos2d);
    GETGLFUN(glWindowPos2dv);
    GETGLFUN(glWindowPos2f);
    GETGLFUN(glWindowPos2fv);
    GETGLFUN(glWindowPos2i);
    GETGLFUN(glWindowPos2iv);
    GETGLFUN(glWindowPos2s);
    GETGLFUN(glWindowPos2sv);
    GETGLFUN(glWindowPos3d);
    GETGLFUN(glWindowPos3dv);
    GETGLFUN(glWindowPos3f);
    GETGLFUN(glWindowPos3fv);
    GETGLFUN(glWindowPos3i);
    GETGLFUN(glWindowPos3iv);
    GETGLFUN(glWindowPos3s);
    GETGLFUN(glWindowPos3sv);
    GETGLFUN(glBlendColor);
    GETGLFUN(glBlendEquation);
    GETGLFUN(glGenQueries);
    GETGLFUN(glDeleteQueries);
    GETGLFUN(glIsQuery);
    GETGLFUN(glBeginQuery);
    GETGLFUN(glEndQuery);
    GETGLFUN(glGetQueryiv);
    GETGLFUN(glGetQueryObjectiv);
    GETGLFUN(glGetQueryObjectuiv);
    GETGLFUN(glBindBuffer);
    GETGLFUN(glDeleteBuffers);
    GETGLFUN(glGenBuffers);
    GETGLFUN(glIsBuffer);
    GETGLFUN(glBufferData);
    GETGLFUN(glBufferSubData);
    GETGLFUN(glGetBufferSubData);
    GETGLFUN(glMapBuffer);
    GETGLFUN(glUnmapBuffer);
    GETGLFUN(glGetBufferParameteriv);
    GETGLFUN(glGetBufferPointerv);
    GETGLFUN(glBlendEquationSeparate);
    GETGLFUN(glDrawBuffers);
    GETGLFUN(glStencilOpSeparate);
    GETGLFUN(glStencilFuncSeparate);
    GETGLFUN(glStencilMaskSeparate);
    GETGLFUN(glAttachShader);
    GETGLFUN(glBindAttribLocation);
    GETGLFUN(glCompileShader);
    GETGLFUN(glCreateProgram);
    GETGLFUN(glCreateShader);
    GETGLFUN(glDeleteProgram);
    GETGLFUN(glDeleteShader);
    GETGLFUN(glDetachShader);
    GETGLFUN(glDisableVertexAttribArray);
    GETGLFUN(glEnableVertexAttribArray);
    GETGLFUN(glGetActiveAttrib);
    GETGLFUN(glGetActiveUniform);
    GETGLFUN(glGetAttachedShaders);
    GETGLFUN(glGetAttribLocation);
    GETGLFUN(glGetProgramiv);
    GETGLFUN(glGetProgramInfoLog);
    GETGLFUN(glGetShaderiv);
    GETGLFUN(glGetShaderInfoLog);
    GETGLFUN(glGetShaderSource);
    GETGLFUN(glGetUniformLocation);
    GETGLFUN(glGetUniformfv);
    GETGLFUN(glGetUniformiv);
    GETGLFUN(glGetVertexAttribdv);
    GETGLFUN(glGetVertexAttribfv);
    GETGLFUN(glGetVertexAttribiv);
    GETGLFUN(glGetVertexAttribPointerv);
    GETGLFUN(glIsProgram);
    GETGLFUN(glIsShader);
    GETGLFUN(glLinkProgram);
    GETGLFUN(glShaderSource);
    GETGLFUN(glUseProgram);
    GETGLFUN(glUniform1f);
    GETGLFUN(glUniform2f);
    GETGLFUN(glUniform3f);
    GETGLFUN(glUniform4f);
    GETGLFUN(glUniform1i);
    GETGLFUN(glUniform2i);
    GETGLFUN(glUniform3i);
    GETGLFUN(glUniform4i);
    GETGLFUN(glUniform1fv);
    GETGLFUN(glUniform2fv);
    GETGLFUN(glUniform3fv);
    GETGLFUN(glUniform4fv);
    GETGLFUN(glUniform1iv);
    GETGLFUN(glUniform2iv);
    GETGLFUN(glUniform3iv);
    GETGLFUN(glUniform4iv);
    GETGLFUN(glUniformMatrix2fv);
    GETGLFUN(glUniformMatrix3fv);
    GETGLFUN(glUniformMatrix4fv);
    GETGLFUN(glValidateProgram);
    GETGLFUN(glVertexAttrib1d);
    GETGLFUN(glVertexAttrib1dv);
    GETGLFUN(glVertexAttrib1f);
    GETGLFUN(glVertexAttrib1fv);
    GETGLFUN(glVertexAttrib1s);
    GETGLFUN(glVertexAttrib1sv);
    GETGLFUN(glVertexAttrib2d);
    GETGLFUN(glVertexAttrib2dv);
    GETGLFUN(glVertexAttrib2f);
    GETGLFUN(glVertexAttrib2fv);
    GETGLFUN(glVertexAttrib2s);
    GETGLFUN(glVertexAttrib2sv);
    GETGLFUN(glVertexAttrib3d);
    GETGLFUN(glVertexAttrib3dv);
    GETGLFUN(glVertexAttrib3f);
    GETGLFUN(glVertexAttrib3fv);
    GETGLFUN(glVertexAttrib3s);
    GETGLFUN(glVertexAttrib3sv);
    GETGLFUN(glVertexAttrib4Nbv);
    GETGLFUN(glVertexAttrib4Niv);
    GETGLFUN(glVertexAttrib4Nsv);
    GETGLFUN(glVertexAttrib4Nub);
    GETGLFUN(glVertexAttrib4Nubv);
    GETGLFUN(glVertexAttrib4Nuiv);
    GETGLFUN(glVertexAttrib4Nusv);
    GETGLFUN(glVertexAttrib4bv);
    GETGLFUN(glVertexAttrib4d);
    GETGLFUN(glVertexAttrib4dv);
    GETGLFUN(glVertexAttrib4f);
    GETGLFUN(glVertexAttrib4fv);
    GETGLFUN(glVertexAttrib4iv);
    GETGLFUN(glVertexAttrib4s);
    GETGLFUN(glVertexAttrib4sv);
    GETGLFUN(glVertexAttrib4ubv);
    GETGLFUN(glVertexAttrib4uiv);
    GETGLFUN(glVertexAttrib4usv);
    GETGLFUN(glVertexAttribPointer);
    GETGLFUN(glUniformMatrix2x3fv);
    GETGLFUN(glUniformMatrix3x2fv);
    GETGLFUN(glUniformMatrix2x4fv);
    GETGLFUN(glUniformMatrix4x2fv);
    GETGLFUN(glUniformMatrix3x4fv);
    GETGLFUN(glUniformMatrix4x3fv);
    GETGLFUN(glColorMaski);
    GETGLFUN(glGetBooleani_v);
    GETGLFUN(glGetIntegeri_v);
    GETGLFUN(glEnablei);
    GETGLFUN(glDisablei);
    GETGLFUN(glIsEnabledi);
    GETGLFUN(glBeginTransformFeedback);
    GETGLFUN(glEndTransformFeedback);
    GETGLFUN(glBindBufferRange);
    GETGLFUN(glBindBufferBase);
    GETGLFUN(glTransformFeedbackVaryings);
    GETGLFUN(glGetTransformFeedbackVarying);
    GETGLFUN(glClampColor);
    GETGLFUN(glBeginConditionalRender);
    GETGLFUN(glEndConditionalRender);
    GETGLFUN(glVertexAttribIPointer);
    GETGLFUN(glGetVertexAttribIiv);
    GETGLFUN(glGetVertexAttribIuiv);
    GETGLFUN(glVertexAttribI1i);
    GETGLFUN(glVertexAttribI2i);
    GETGLFUN(glVertexAttribI3i);
    GETGLFUN(glVertexAttribI4i);
    GETGLFUN(glVertexAttribI1ui);
    GETGLFUN(glVertexAttribI2ui);
    GETGLFUN(glVertexAttribI3ui);
    GETGLFUN(glVertexAttribI4ui);
    GETGLFUN(glVertexAttribI1iv);
    GETGLFUN(glVertexAttribI2iv);
    GETGLFUN(glVertexAttribI3iv);
    GETGLFUN(glVertexAttribI4iv);
    GETGLFUN(glVertexAttribI1uiv);
    GETGLFUN(glVertexAttribI2uiv);
    GETGLFUN(glVertexAttribI3uiv);
    GETGLFUN(glVertexAttribI4uiv);
    GETGLFUN(glVertexAttribI4bv);
    GETGLFUN(glVertexAttribI4sv);
    GETGLFUN(glVertexAttribI4ubv);
    GETGLFUN(glVertexAttribI4usv);
    GETGLFUN(glGetUniformuiv);
    GETGLFUN(glBindFragDataLocation);
    GETGLFUN(glGetFragDataLocation);
    GETGLFUN(glUniform1ui);
    GETGLFUN(glUniform2ui);
    GETGLFUN(glUniform3ui);
    GETGLFUN(glUniform4ui);
    GETGLFUN(glUniform1uiv);
    GETGLFUN(glUniform2uiv);
    GETGLFUN(glUniform3uiv);
    GETGLFUN(glUniform4uiv);
    GETGLFUN(glTexParameterIiv);
    GETGLFUN(glTexParameterIuiv);
    GETGLFUN(glGetTexParameterIiv);
    GETGLFUN(glGetTexParameterIuiv);
    GETGLFUN(glClearBufferiv);
    GETGLFUN(glClearBufferuiv);
    GETGLFUN(glClearBufferfv);
    GETGLFUN(glClearBufferfi);
    GETGLFUN(glIsRenderbuffer);
    GETGLFUN(glBindRenderbuffer);
    GETGLFUN(glDeleteRenderbuffers);
    GETGLFUN(glGenRenderbuffers);
    GETGLFUN(glRenderbufferStorage);
    GETGLFUN(glGetRenderbufferParameteriv);
    GETGLFUN(glIsFramebuffer);
    GETGLFUN(glBindFramebuffer);
    GETGLFUN(glDeleteFramebuffers);
    GETGLFUN(glGenFramebuffers);
    GETGLFUN(glCheckFramebufferStatus);
    GETGLFUN(glFramebufferTexture1D);
    GETGLFUN(glFramebufferTexture2D);
    GETGLFUN(glFramebufferTexture3D);
    GETGLFUN(glFramebufferRenderbuffer);
    GETGLFUN(glGetFramebufferAttachmentParameteriv);
    GETGLFUN(glGenerateMipmap);
    GETGLFUN(glBlitFramebuffer);
    GETGLFUN(glRenderbufferStorageMultisample);
    GETGLFUN(glFramebufferTextureLayer);
    GETGLFUN(glMapBufferRange);
    GETGLFUN(glFlushMappedBufferRange);
    GETGLFUN(glBindVertexArray);
    GETGLFUN(glDeleteVertexArrays);
    GETGLFUN(glGenVertexArrays);
    GETGLFUN(glIsVertexArray);
    GETGLFUN(glDrawArraysInstanced);
    GETGLFUN(glDrawElementsInstanced);
    GETGLFUN(glTexBuffer);
    GETGLFUN(glPrimitiveRestartIndex);
    GETGLFUN(glCopyBufferSubData);
    GETGLFUN(glGetUniformIndices);
    GETGLFUN(glGetActiveUniformsiv);
    GETGLFUN(glGetActiveUniformName);
    GETGLFUN(glGetUniformBlockIndex);
    GETGLFUN(glGetActiveUniformBlockiv);
    GETGLFUN(glGetActiveUniformBlockName);
    GETGLFUN(glUniformBlockBinding);
    GETGLFUN(glDrawElementsBaseVertex);
    GETGLFUN(glDrawRangeElementsBaseVertex);
    GETGLFUN(glDrawElementsInstancedBaseVertex);
    GETGLFUN(glMultiDrawElementsBaseVertex);
    GETGLFUN(glProvokingVertex);
    GETGLFUN(glFenceSync);
    GETGLFUN(glIsSync);
    GETGLFUN(glDeleteSync);
    GETGLFUN(glClientWaitSync);
    GETGLFUN(glWaitSync);
    GETGLFUN(glGetInteger64v);
    GETGLFUN(glGetSynciv);
    GETGLFUN(glGetInteger64i_v);
    GETGLFUN(glGetBufferParameteri64v);
    GETGLFUN(glFramebufferTexture);
    GETGLFUN(glTexImage2DMultisample);
    GETGLFUN(glTexImage3DMultisample);
    GETGLFUN(glGetMultisamplefv);
    GETGLFUN(glSampleMaski);
    GETGLFUN(glBindFragDataLocationIndexed);
    GETGLFUN(glGetFragDataIndex);
    GETGLFUN(glGenSamplers);
    GETGLFUN(glDeleteSamplers);
    GETGLFUN(glIsSampler);
    GETGLFUN(glBindSampler);
    GETGLFUN(glSamplerParameteri);
    GETGLFUN(glSamplerParameteriv);
    GETGLFUN(glSamplerParameterf);
    GETGLFUN(glSamplerParameterfv);
    GETGLFUN(glSamplerParameterIiv);
    GETGLFUN(glSamplerParameterIuiv);
    GETGLFUN(glGetSamplerParameteriv);
    GETGLFUN(glGetSamplerParameterIiv);
    GETGLFUN(glGetSamplerParameterfv);
    GETGLFUN(glGetSamplerParameterIuiv);
    GETGLFUN(glQueryCounter);
    GETGLFUN(glGetQueryObjecti64v);
    GETGLFUN(glGetQueryObjectui64v);
    GETGLFUN(glVertexAttribDivisor);
    GETGLFUN(glVertexAttribP1ui);
    GETGLFUN(glVertexAttribP1uiv);
    GETGLFUN(glVertexAttribP2ui);
    GETGLFUN(glVertexAttribP2uiv);
    GETGLFUN(glVertexAttribP3ui);
    GETGLFUN(glVertexAttribP3uiv);
    GETGLFUN(glVertexAttribP4ui);
    GETGLFUN(glVertexAttribP4uiv);
    GETGLFUN(glVertexP2ui);
    GETGLFUN(glVertexP2uiv);
    GETGLFUN(glVertexP3ui);
    GETGLFUN(glVertexP3uiv);
    GETGLFUN(glVertexP4ui);
    GETGLFUN(glVertexP4uiv);
    GETGLFUN(glTexCoordP1ui);
    GETGLFUN(glTexCoordP1uiv);
    GETGLFUN(glTexCoordP2ui);
    GETGLFUN(glTexCoordP2uiv);
    GETGLFUN(glTexCoordP3ui);
    GETGLFUN(glTexCoordP3uiv);
    GETGLFUN(glTexCoordP4ui);
    GETGLFUN(glTexCoordP4uiv);
    GETGLFUN(glMultiTexCoordP1ui);
    GETGLFUN(glMultiTexCoordP1uiv);
    GETGLFUN(glMultiTexCoordP2ui);
    GETGLFUN(glMultiTexCoordP2uiv);
    GETGLFUN(glMultiTexCoordP3ui);
    GETGLFUN(glMultiTexCoordP3uiv);
    GETGLFUN(glMultiTexCoordP4ui);
    GETGLFUN(glMultiTexCoordP4uiv);
    GETGLFUN(glNormalP3ui);
    GETGLFUN(glNormalP3uiv);
    GETGLFUN(glColorP3ui);
    GETGLFUN(glColorP3uiv);
    GETGLFUN(glColorP4ui);
    GETGLFUN(glColorP4uiv);
    GETGLFUN(glSecondaryColorP3ui);
    GETGLFUN(glSecondaryColorP3uiv);
    GETGLFUN(glMinSampleShading);
    GETGLFUN(glBlendEquationi);
    GETGLFUN(glBlendEquationSeparatei);
    GETGLFUN(glBlendFunci);
    GETGLFUN(glBlendFuncSeparatei);
    GETGLFUN(glDrawArraysIndirect);
    GETGLFUN(glDrawElementsIndirect);
    GETGLFUN(glUniform1d);
    GETGLFUN(glUniform2d);
    GETGLFUN(glUniform3d);
    GETGLFUN(glUniform4d);
    GETGLFUN(glUniform1dv);
    GETGLFUN(glUniform2dv);
    GETGLFUN(glUniform3dv);
    GETGLFUN(glUniform4dv);
    GETGLFUN(glUniformMatrix2dv);
    GETGLFUN(glUniformMatrix3dv);
    GETGLFUN(glUniformMatrix4dv);
    GETGLFUN(glUniformMatrix2x3dv);
    GETGLFUN(glUniformMatrix2x4dv);
    GETGLFUN(glUniformMatrix3x2dv);
    GETGLFUN(glUniformMatrix3x4dv);
    GETGLFUN(glUniformMatrix4x2dv);
    GETGLFUN(glUniformMatrix4x3dv);
    GETGLFUN(glGetUniformdv);
    GETGLFUN(glGetSubroutineUniformLocation);
    GETGLFUN(glGetSubroutineIndex);
    GETGLFUN(glGetActiveSubroutineUniformiv);
    GETGLFUN(glGetActiveSubroutineUniformName);
    GETGLFUN(glGetActiveSubroutineName);
    GETGLFUN(glUniformSubroutinesuiv);
    GETGLFUN(glGetUniformSubroutineuiv);
    GETGLFUN(glGetProgramStageiv);
    GETGLFUN(glPatchParameteri);
    GETGLFUN(glPatchParameterfv);
    GETGLFUN(glBindTransformFeedback);
    GETGLFUN(glDeleteTransformFeedbacks);
    GETGLFUN(glGenTransformFeedbacks);
    GETGLFUN(glIsTransformFeedback);
    GETGLFUN(glPauseTransformFeedback);
    GETGLFUN(glResumeTransformFeedback);
    GETGLFUN(glDrawTransformFeedback);
    GETGLFUN(glDrawTransformFeedbackStream);
    GETGLFUN(glBeginQueryIndexed);
    GETGLFUN(glEndQueryIndexed);
    GETGLFUN(glGetQueryIndexediv);
    GETGLFUN(glReleaseShaderCompiler);
    GETGLFUN(glShaderBinary);
    GETGLFUN(glGetShaderPrecisionFormat);
    GETGLFUN(glDepthRangef);
    GETGLFUN(glClearDepthf);
    GETGLFUN(glGetProgramBinary);
    GETGLFUN(glProgramBinary);
    GETGLFUN(glProgramParameteri);
    GETGLFUN(glUseProgramStages);
    GETGLFUN(glActiveShaderProgram);
    GETGLFUN(glCreateShaderProgramv);
    GETGLFUN(glBindProgramPipeline);
    GETGLFUN(glDeleteProgramPipelines);
    GETGLFUN(glGenProgramPipelines);
    GETGLFUN(glIsProgramPipeline);
    GETGLFUN(glGetProgramPipelineiv);
    GETGLFUN(glProgramUniform1i);
    GETGLFUN(glProgramUniform1iv);
    GETGLFUN(glProgramUniform1f);
    GETGLFUN(glProgramUniform1fv);
    GETGLFUN(glProgramUniform1d);
    GETGLFUN(glProgramUniform1dv);
    GETGLFUN(glProgramUniform1ui);
    GETGLFUN(glProgramUniform1uiv);
    GETGLFUN(glProgramUniform2i);
    GETGLFUN(glProgramUniform2iv);
    GETGLFUN(glProgramUniform2f);
    GETGLFUN(glProgramUniform2fv);
    GETGLFUN(glProgramUniform2d);
    GETGLFUN(glProgramUniform2dv);
    GETGLFUN(glProgramUniform2ui);
    GETGLFUN(glProgramUniform2uiv);
    GETGLFUN(glProgramUniform3i);
    GETGLFUN(glProgramUniform3iv);
    GETGLFUN(glProgramUniform3f);
    GETGLFUN(glProgramUniform3fv);
    GETGLFUN(glProgramUniform3d);
    GETGLFUN(glProgramUniform3dv);
    GETGLFUN(glProgramUniform3ui);
    GETGLFUN(glProgramUniform3uiv);
    GETGLFUN(glProgramUniform4i);
    GETGLFUN(glProgramUniform4iv);
    GETGLFUN(glProgramUniform4f);
    GETGLFUN(glProgramUniform4fv);
    GETGLFUN(glProgramUniform4d);
    GETGLFUN(glProgramUniform4dv);
    GETGLFUN(glProgramUniform4ui);
    GETGLFUN(glProgramUniform4uiv);
    GETGLFUN(glProgramUniformMatrix2fv);
    GETGLFUN(glProgramUniformMatrix3fv);
    GETGLFUN(glProgramUniformMatrix4fv);
    GETGLFUN(glProgramUniformMatrix2dv);
    GETGLFUN(glProgramUniformMatrix3dv);
    GETGLFUN(glProgramUniformMatrix4dv);
    GETGLFUN(glProgramUniformMatrix2x3fv);
    GETGLFUN(glProgramUniformMatrix3x2fv);
    GETGLFUN(glProgramUniformMatrix2x4fv);
    GETGLFUN(glProgramUniformMatrix4x2fv);
    GETGLFUN(glProgramUniformMatrix3x4fv);
    GETGLFUN(glProgramUniformMatrix4x3fv);
    GETGLFUN(glProgramUniformMatrix2x3dv);
    GETGLFUN(glProgramUniformMatrix3x2dv);
    GETGLFUN(glProgramUniformMatrix2x4dv);
    GETGLFUN(glProgramUniformMatrix4x2dv);
    GETGLFUN(glProgramUniformMatrix3x4dv);
    GETGLFUN(glProgramUniformMatrix4x3dv);
    GETGLFUN(glValidateProgramPipeline);
    GETGLFUN(glGetProgramPipelineInfoLog);
    GETGLFUN(glVertexAttribL1d);
    GETGLFUN(glVertexAttribL2d);
    GETGLFUN(glVertexAttribL3d);
    GETGLFUN(glVertexAttribL4d);
    GETGLFUN(glVertexAttribL1dv);
    GETGLFUN(glVertexAttribL2dv);
    GETGLFUN(glVertexAttribL3dv);
    GETGLFUN(glVertexAttribL4dv);
    GETGLFUN(glVertexAttribLPointer);
    GETGLFUN(glGetVertexAttribLdv);
    GETGLFUN(glViewportArrayv);
    GETGLFUN(glViewportIndexedf);
    GETGLFUN(glViewportIndexedfv);
    GETGLFUN(glScissorArrayv);
    GETGLFUN(glScissorIndexed);
    GETGLFUN(glScissorIndexedv);
    GETGLFUN(glDepthRangeArrayv);
    GETGLFUN(glDepthRangeIndexed);
    GETGLFUN(glGetFloati_v);
    GETGLFUN(glGetDoublei_v);
    GETGLFUN(glDrawArraysInstancedBaseInstance);
    GETGLFUN(glDrawElementsInstancedBaseInstance);
    GETGLFUN(glDrawElementsInstancedBaseVertexBaseInstance);
    GETGLFUN(glGetInternalformativ);
    GETGLFUN(glGetActiveAtomicCounterBufferiv);
    GETGLFUN(glBindImageTexture);
    GETGLFUN(glMemoryBarrier);
    GETGLFUN(glTexStorage1D);
    GETGLFUN(glTexStorage2D);
    GETGLFUN(glTexStorage3D);
    GETGLFUN(glDrawTransformFeedbackInstanced);
    GETGLFUN(glDrawTransformFeedbackStreamInstanced);
    GETGLFUN(glClearBufferData);
    GETGLFUN(glClearBufferSubData);
    GETGLFUN(glDispatchCompute);
    GETGLFUN(glDispatchComputeIndirect);
    GETGLFUN(glFramebufferParameteri);
    GETGLFUN(glGetFramebufferParameteriv);
    GETGLFUN(glGetInternalformati64v);
    GETGLFUN(glInvalidateTexSubImage);
    GETGLFUN(glInvalidateTexImage);
    GETGLFUN(glInvalidateBufferSubData);
    GETGLFUN(glInvalidateBufferData);
    GETGLFUN(glInvalidateFramebuffer);
    GETGLFUN(glInvalidateSubFramebuffer);
    GETGLFUN(glMultiDrawArraysIndirect);
    GETGLFUN(glMultiDrawElementsIndirect);
    GETGLFUN(glGetProgramInterfaceiv);
    GETGLFUN(glGetProgramResourceIndex);
    GETGLFUN(glGetProgramResourceName);
    GETGLFUN(glGetProgramResourceiv);
    GETGLFUN(glGetProgramResourceLocation);
    GETGLFUN(glGetProgramResourceLocationIndex);
    GETGLFUN(glShaderStorageBlockBinding);
    GETGLFUN(glTexBufferRange);
    GETGLFUN(glTexStorage2DMultisample);
    GETGLFUN(glTexStorage3DMultisample);
    GETGLFUN(glTextureView);
    GETGLFUN(glBindVertexBuffer);
    GETGLFUN(glVertexAttribFormat);
    GETGLFUN(glVertexAttribIFormat);
    GETGLFUN(glVertexAttribLFormat);
    GETGLFUN(glVertexAttribBinding);
    GETGLFUN(glVertexBindingDivisor);
    GETGLFUN(glDebugMessageControl);
    GETGLFUN(glDebugMessageInsert);
    GETGLFUN(glDebugMessageCallback);
    GETGLFUN(glGetDebugMessageLog);
    GETGLFUN(glPushDebugGroup);
    GETGLFUN(glPopDebugGroup);
    GETGLFUN(glObjectLabel);
    GETGLFUN(glGetObjectLabel);
    GETGLFUN(glObjectPtrLabel);
    GETGLFUN(glGetObjectPtrLabel);
    GETGLFUN(glClipControl);
    GETGLFUN(glCreateTransformFeedbacks);
    GETGLFUN(glTransformFeedbackBufferBase);
    GETGLFUN(glTransformFeedbackBufferRange);
    GETGLFUN(glGetTransformFeedbackiv);
    GETGLFUN(glGetTransformFeedbacki_v);
    GETGLFUN(glGetTransformFeedbacki64_v);
    GETGLFUN(glCreateBuffers);
    GETGLFUN(glNamedBufferStorage);
    GETGLFUN(glNamedBufferData);
    GETGLFUN(glNamedBufferSubData);
    GETGLFUN(glCopyNamedBufferSubData);
    GETGLFUN(glClearNamedBufferData);
    GETGLFUN(glClearNamedBufferSubData);
    GETGLFUN(glMapNamedBuffer);
    GETGLFUN(glMapNamedBufferRange);
    GETGLFUN(glUnmapNamedBuffer);
    GETGLFUN(glFlushMappedNamedBufferRange);
    GETGLFUN(glGetNamedBufferParameteriv);
    GETGLFUN(glGetNamedBufferParameteri64v);
    GETGLFUN(glGetNamedBufferPointerv);
    GETGLFUN(glGetNamedBufferSubData);
    GETGLFUN(glCreateFramebuffers);
    GETGLFUN(glNamedFramebufferRenderbuffer);
    GETGLFUN(glNamedFramebufferParameteri);
    GETGLFUN(glNamedFramebufferTexture);
    GETGLFUN(glNamedFramebufferTextureLayer);
    GETGLFUN(glNamedFramebufferDrawBuffer);
    GETGLFUN(glNamedFramebufferDrawBuffers);
    GETGLFUN(glNamedFramebufferReadBuffer);
    GETGLFUN(glInvalidateNamedFramebufferData);
    GETGLFUN(glInvalidateNamedFramebufferSubData);
    GETGLFUN(glClearNamedFramebufferiv);
    GETGLFUN(glClearNamedFramebufferuiv);
    GETGLFUN(glClearNamedFramebufferfv);
    GETGLFUN(glClearNamedFramebufferfi);
    GETGLFUN(glBlitNamedFramebuffer);
    GETGLFUN(glCheckNamedFramebufferStatus);
    GETGLFUN(glGetNamedFramebufferParameteriv);
    GETGLFUN(glGetNamedFramebufferAttachmentParameteriv);
    GETGLFUN(glCreateRenderbuffers);
    GETGLFUN(glNamedRenderbufferStorage);
    GETGLFUN(glNamedRenderbufferStorageMultisample);
    GETGLFUN(glGetNamedRenderbufferParameteriv);
    GETGLFUN(glCreateTextures);
    GETGLFUN(glTextureBuffer);
    GETGLFUN(glTextureBufferRange);
    GETGLFUN(glTextureStorage1D);
    GETGLFUN(glTextureStorage2D);
    GETGLFUN(glTextureStorage3D);
    GETGLFUN(glTextureStorage2DMultisample);
    GETGLFUN(glTextureStorage3DMultisample);
    GETGLFUN(glTextureSubImage1D);
    GETGLFUN(glTextureSubImage2D);
    GETGLFUN(glTextureSubImage3D);
    GETGLFUN(glCompressedTextureSubImage1D);
    GETGLFUN(glCompressedTextureSubImage2D);
    GETGLFUN(glCompressedTextureSubImage3D);
    GETGLFUN(glCopyTextureSubImage1D);
    GETGLFUN(glCopyTextureSubImage2D);
    GETGLFUN(glCopyTextureSubImage3D);
    GETGLFUN(glTextureParameterf);
    GETGLFUN(glTextureParameterfv);
    GETGLFUN(glTextureParameteri);
    GETGLFUN(glTextureParameterIiv);
    GETGLFUN(glTextureParameterIuiv);
    GETGLFUN(glTextureParameteriv);
    GETGLFUN(glGenerateTextureMipmap);
    GETGLFUN(glBindTextureUnit);
    GETGLFUN(glGetTextureImage);
    GETGLFUN(glGetCompressedTextureImage);
    GETGLFUN(glGetTextureLevelParameterfv);
    GETGLFUN(glGetTextureLevelParameteriv);
    GETGLFUN(glGetTextureParameterfv);
    GETGLFUN(glGetTextureParameterIiv);
    GETGLFUN(glGetTextureParameterIuiv);
    GETGLFUN(glGetTextureParameteriv);
    GETGLFUN(glCreateVertexArrays);
    GETGLFUN(glDisableVertexArrayAttrib);
    GETGLFUN(glEnableVertexArrayAttrib);
    GETGLFUN(glVertexArrayElementBuffer);
    GETGLFUN(glVertexArrayVertexBuffer);
    GETGLFUN(glVertexArrayVertexBuffers);
    GETGLFUN(glVertexArrayAttribBinding);
    GETGLFUN(glVertexArrayAttribFormat);
    GETGLFUN(glVertexArrayAttribIFormat);
    GETGLFUN(glVertexArrayAttribLFormat);
    GETGLFUN(glVertexArrayBindingDivisor);
    GETGLFUN(glGetVertexArrayiv);
    GETGLFUN(glGetVertexArrayIndexediv);
    GETGLFUN(glGetVertexArrayIndexed64iv);
    GETGLFUN(glCreateSamplers);
    GETGLFUN(glCreateProgramPipelines);
    GETGLFUN(glCreateQueries);
    GETGLFUN(glGetQueryBufferObjecti64v);
    GETGLFUN(glGetQueryBufferObjectiv);
    GETGLFUN(glGetQueryBufferObjectui64v);
    GETGLFUN(glGetQueryBufferObjectuiv);
    GETGLFUN(glMemoryBarrierByRegion);
    GETGLFUN(glGetTextureSubImage);
    GETGLFUN(glGetCompressedTextureSubImage);
    GETGLFUN(glGetGraphicsResetStatus);
    GETGLFUN(glGetnCompressedTexImage);
    GETGLFUN(glGetnTexImage);
    GETGLFUN(glGetnUniformdv);
    GETGLFUN(glGetnUniformfv);
    GETGLFUN(glGetnUniformiv);
    GETGLFUN(glGetnUniformuiv);
    GETGLFUN(glReadnPixels);
    GETGLFUN(glGetnMapdv);
    GETGLFUN(glGetnMapfv);
    GETGLFUN(glGetnMapiv);
    GETGLFUN(glGetnPixelMapfv);
    GETGLFUN(glGetnPixelMapuiv);
    GETGLFUN(glGetnPixelMapusv);
    GETGLFUN(glGetnPolygonStipple);
    GETGLFUN(glGetnColorTable);
    GETGLFUN(glGetnConvolutionFilter);
    GETGLFUN(glGetnSeparableFilter);
    GETGLFUN(glGetnHistogram);
    GETGLFUN(glGetnMinmax);
    GETGLFUN(glTextureBarrier);
    GETGLFUN(glSpecializeShader);
    GETGLFUN(glMultiDrawArraysIndirectCount);
    GETGLFUN(glMultiDrawElementsIndirectCount);
    GETGLFUN(glPolygonOffsetClamp);
}

// NOTE(Dima): Thanks https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c
HGLRC Win32InitOpenGL(HDC realDC){
    Win32LoadOpenglExtensions();
    
    const int pFormatAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
        WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,         32,
        WGL_DEPTH_BITS_ARB,         24,
        WGL_STENCIL_BITS_ARB,       8,
        0,
    };
    
    int pFormat;
    UINT nFormats = 0;
    wglChoosePixelFormatARB(realDC, pFormatAttribs, 0, 1, &pFormat, &nFormats);
    Assert(nFormats);
    
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(realDC, pFormat, sizeof(pfd), &pfd);
    BOOL setPFResult = SetPixelFormat(realDC, pFormat, &pfd);
    Assert(setPFResult);
    
    // Specify that we want to create an OpenGL 3.3 core profile context
    const int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    
    HGLRC resCtx = wglCreateContextAttribsARB(realDC, 0, attribs);
    Assert(resCtx);
    
    BOOL MakeCurrentResult = wglMakeCurrent(realDC, resCtx);
    Assert(MakeCurrentResult);
    
    wglSwapIntervalEXT(0);
    
    Win32GetOpenglFunctions();
    
    return(resCtx);
}

void Win32FreeOpenGL(HGLRC renderCtx){
    wglDeleteContext(renderCtx);
}

PLATFORM_MUTEX_FUNCTION(Win32InitMutex){
    win_critical_section_slot* Slot = 0;
    ASSERT(GlobalWin32.NotUsedCriticalSectionIndicesCount);
    
    int Index = GlobalWin32.NotUsedCriticalSectionIndices[0];
    int EndIndex = GlobalWin32.NotUsedCriticalSectionIndicesCount - 1;
    
    Slot = &GlobalWin32.CriticalSections[Index];
    
    // NOTE(Dima): Swapping with last
    GlobalWin32.NotUsedCriticalSectionIndices[0] = GlobalWin32.NotUsedCriticalSectionIndices[EndIndex];
    // NOTE(Dima): Setting last to invalid value
    GlobalWin32.NotUsedCriticalSectionIndices[EndIndex] = 0xBEEF;
    // NOTE(Dima): Decrementing array size
    --GlobalWin32.NotUsedCriticalSectionIndicesCount;
    
    CRITICAL_SECTION* Section = &Slot->Section;
    
    // NOTE(Dima): Setting NativeHandle to index so we can get section in future
    Mutex->NativeHandle = Index;
    
    // NOTE(Dima): Initializing critical section
    InitializeCriticalSection(Section);
}

PLATFORM_MUTEX_FUNCTION(Win32LockMutex){
    win_critical_section_slot* Slot = &GlobalWin32.CriticalSections[Mutex->NativeHandle];
    CRITICAL_SECTION* Section = &Slot->Section;
    
    EnterCriticalSection(Section);
}

PLATFORM_MUTEX_FUNCTION(Win32UnlockMutex){
    win_critical_section_slot* Slot = &GlobalWin32.CriticalSections[Mutex->NativeHandle];
    CRITICAL_SECTION* Section = &Slot->Section;
    
    LeaveCriticalSection(Section);
}

PLATFORM_MUTEX_FUNCTION(Win32FreeMutex){
    ASSERT(Mutex->NativeHandle != 0xDEADBEEF);
    
    win_critical_section_slot* Slot = &GlobalWin32.CriticalSections[Mutex->NativeHandle];
    CRITICAL_SECTION* Section = &Slot->Section;
    
    int* Count = &GlobalWin32.NotUsedCriticalSectionIndicesCount;
    ASSERT(*Count < MAX_CRITICAL_SECTIONS_COUNT);
    GlobalWin32.NotUsedCriticalSectionIndices[(*Count)++] = Mutex->NativeHandle;
    
    // NOTE(Dima): Clearing mutex native handle as we do not need it anymore
    Mutex->NativeHandle = 0xDEADBEEF;
    
    DeleteCriticalSection(Section);
}

PLATFORM_ADD_ENTRY(PlatformAddEntry){
    Win32LockMutex(&queue->AddMutex);
    
    uint32_t oldAddIndex = queue->AddIndex.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    
    uint32_t newAddIndex = (oldAddIndex + 1) % queue->JobsCount;
    // NOTE(Dima): We should not overlap
    Assert(newAddIndex != queue->DoIndex.load(std::memory_order_acquire));
    
    platform_job* job = &queue->Jobs[oldAddIndex];
    job->Callback = callback;
    job->Data = data;
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->AddIndex.store(newAddIndex, std::memory_order_relaxed);
    queue->Started.fetch_add(1);
    
    queue->ConditionVariable.notify_all();
    
    Win32UnlockMutex(&queue->AddMutex);
}

INTERNAL_FUNCTION b32 PlatformDoWorkerWork(platform_job_queue* queue){
    b32 res = 0;
    
    std::uint32_t d = queue->DoIndex.load();
    
    if(d != queue->AddIndex.load(std::memory_order_acquire)){
        std::uint32_t newD = (d + 1) % queue->JobsCount;
        if(queue->DoIndex.compare_exchange_weak(d, newD)){
            platform_job* job = &queue->Jobs[d];
            
            job->Callback(job->Data);
            
            queue->Finished.fetch_add(1);
        }
        else{
            // NOTE(Dima): Value has not been changed because of spuorious failure
        }
    }
    else{
        res = 1;
    }
    
    return(res);
}

INTERNAL_FUNCTION void PlatformWorkerThread(platform_job_queue* queue){
    for(;;){
        if(PlatformDoWorkerWork(queue)){
            std::unique_lock<std::mutex> UniqueLock(queue->ConditionVariableMutex);
            queue->ConditionVariable.wait(UniqueLock);
        }
    }
}

PLATFORM_WAIT_FOR_COMPLETION(PlatformWaitForCompletion){
    while(queue->Started.load() != queue->Finished.load())
    {
        PlatformDoWorkerWork(queue);
    }
    
    std::atomic_thread_fence(std::memory_order_release);
    queue->Started.store(0, std::memory_order_relaxed);
    queue->Finished.store(0, std::memory_order_relaxed);
}

INTERNAL_FUNCTION void InitJobQueue(platform_job_queue* queue, int jobsCount, int threadCount){
    Win32InitMutex(&queue->AddMutex);
    
    queue->AddIndex.store(0, std::memory_order_relaxed);
    queue->DoIndex.store(0, std::memory_order_relaxed);
    
    queue->Started.store(0, std::memory_order_relaxed);
    queue->Finished.store(0, std::memory_order_relaxed);
    
    queue->Jobs = (platform_job*)malloc(jobsCount * sizeof(platform_job));
    queue->JobsCount = jobsCount;
    
    for(int jobIndex = 0; jobIndex < jobsCount; jobIndex++){
        platform_job* job = queue->Jobs + jobIndex;
        
        job->Callback = 0;
        job->Data = 0;
    }
    
    queue->Threads.reserve(threadCount);
    for(int threadIndex = 0;
        threadIndex < threadCount;
        threadIndex++)
    {
#if 1
        queue->Threads.push_back(std::thread(PlatformWorkerThread, queue));
        queue->Threads[threadIndex].detach();
#else
        std::thread newThread(PlatformWorkerThread, queue);
        newThread.detach();
#endif
    }
}

INTERNAL_FUNCTION void FreeJobQueue(platform_job_queue* queue){
    if(queue->Jobs){
        free(queue->Jobs);
    }
    queue->Jobs = 0;
    queue->Threads.clear();
    Win32FreeMutex(&queue->AddMutex);
}

PLATFORM_FREE_FILE_MEMORY(Win32FreeFileMemory){
    if (fileReadResult->data != 0){
        VirtualFree(fileReadResult->data, 0, MEM_RELEASE);
    }
    
    fileReadResult->data = 0;
    fileReadResult->dataSize = 0;
}

PLATFORM_READ_FILE(Win32ReadFile){
    Platform_Read_File_Result res = {};
    
    HANDLE fileHandle = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0, 0);
    
    if (fileHandle != INVALID_HANDLE_VALUE){
        LARGE_INTEGER fileSizeLI;
        if (GetFileSizeEx(fileHandle, &fileSizeLI)){
            u32 fileSize = (fileSizeLI.QuadPart & 0xFFFFFFFF);
            res.data = VirtualAlloc(0, fileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (res.data){
                DWORD bytesRead;
                if (ReadFile(fileHandle, res.data, fileSize, &bytesRead, 0) && (fileSize == bytesRead)){
                    res.dataSize = fileSize;
                }
                else{
                    Win32FreeFileMemory(&res);
                }
            }
            else{
                //TODO(Dima): Logging. Can not allocate memory
            }
        }
        else{
            //TODO(Dima): Logging. Can't get file size
        }
        
        CloseHandle(fileHandle);
    }
    else{
        //TODO(Dima): Logging. Can't open file
    }
    
    return(res);
}

PLATFORM_WRITE_FILE(Win32WriteFile){
    b32 result = 0;
    
    HANDLE fileHandle = CreateFileA(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE){
        DWORD bytesWritten;
        if (WriteFile(fileHandle, data, size, &bytesWritten, 0)){
            result = (bytesWritten == size);
        }
        else{
            //TODO(Dima): Logging
        }
        
        CloseHandle(fileHandle);
    }
    else{
        //TODO(Dima): Logging
    }
    
    return(result);
}

PLATFORM_SHOW_ERROR(Win32ShowError){
    char* captionText = "Error";
    u32 mbType = MB_OK;
    
    switch(type){
        case PlatformError_Error:{
            captionText = "Error";
            mbType |= MB_ICONERROR;
        }break;
        
        case PlatformError_Warning:{
            captionText = "Warning";
            mbType |= MB_ICONWARNING;
        }break;
        
        case PlatformError_Information:{
            captionText = "Information";
            mbType |= MB_ICONINFORMATION;
        }break;
    }
    
    MessageBoxA(0, text, captionText, mbType);
}

PLATFORM_DEBUG_OUTPUT_STRING(Win32DebugOutputString){
    GlobalWin32.DebugOutputFunc(text);
    GlobalWin32.DebugOutputFunc("\n");
}

PLATFORM_MEMALLOC(Win32MemAlloc){
    
    // NOTE(Dima): I'll allocate 1 extra page to hold info
    // NOTE(Dima): about allocation & win32_memory_block structure
    const mi PageSize = 4096;
    const mi PageSizeMask = 4095;
    // NOTE(Dima): 1 extra info page && 2 guard pages
    mi ToAllocSize = Size + PageSize + PageSize * 2;
    
    void* AllocatedMemory = VirtualAlloc(0, ToAllocSize, 
                                         MEM_COMMIT | MEM_RESERVE, 
                                         PAGE_READWRITE);
    Assert(AllocatedMemory);
    
    Win_Memory_Region* Region = (Win_Memory_Region*)AllocatedMemory;
    
    void* BeginGuardPage = (u8*)Region + PageSize;
    DWORD OldProtectBegin;
    VirtualProtect(BeginGuardPage, PageSize, PAGE_NOACCESS, &OldProtectBegin);
    
    void* BlockBase = (u8*)BeginGuardPage + PageSize;
    
    void* EndGuardPage = (u8*)BlockBase + ((Size + PageSizeMask) & (~(PageSizeMask)));
    DWORD OldProtectEnd;
    VirtualProtect(EndGuardPage, PageSize, PAGE_NOACCESS, &OldProtectEnd);
    
    // NOTE(Dima): Inserting region to list
    Region->Prev = &GlobalWin32.memorySentinel;
    BeginTicketMutex(&GlobalWin32.memoryMutex);
    Region->Next = GlobalWin32.memorySentinel.Next;
    
    Region->Prev->Next = Region;
    Region->Next->Prev = Region;
    EndTicketMutex(&GlobalWin32.memoryMutex);
    
    // NOTE(Dima): Initializing region
    Region->TotalCommittedSize = (ToAllocSize + PageSizeMask) & (~PageSizeMask);
    
    mem_block_entry* Result = (mem_block_entry*)Region;
    Result->Block.Base = BlockBase;
    Result->Block.Used = 0;
    Result->Block.Total = Size;
    
    return(Result);
}

PLATFORM_MEMFREE(Win32MemFree){
    if(ToFree){
        const mi PageSize = 4096;
        
        Win_Memory_Region* Region = (Win_Memory_Region*)ToFree;
        
        // NOTE(Dima): Removing from list
        BeginTicketMutex(&GlobalWin32.memoryMutex);
        Region->Next->Prev = Region->Prev;
        Region->Prev->Next = Region->Next;
        EndTicketMutex(&GlobalWin32.memoryMutex);
        
        VirtualFree(ToFree, 0, MEM_RELEASE);
    }
}

PLATFORM_MEMZERO(Win32MemZero){
    if(ToZero){
        ZeroMemory(ToZero->Block.Base, ToZero->Block.Total);
    }
}


PLATFORM_MEMZERO_RAW(Win32MemZeroRaw){
    if(Data){
        ZeroMemory(Data, DataSize);
    }
}

PLATFORM_OPEN_FILES_BEGIN(Win32OpenFilesBegin){
    ASSERT(GlobalWin32.InOpenFilesBlock == 0);
    GlobalWin32.InOpenFilesBlock = 1;
    
    char NewDirPath[MAX_PATH];
    CopyStrings(NewDirPath, DirectoryPath);
    ChangeAllChars(NewDirPath, '\\', '/');
    int DirPathLen = StringLength(NewDirPath);
    char* LastSymbol = &NewDirPath[DirPathLen - 1];
    if((*LastSymbol != '/') && (DirPathLen - 1 >= 0)){
        LastSymbol++;
        *LastSymbol++ = '/';
        *LastSymbol = 0;
    }
    
    CopyStrings(GlobalWin32.OpenFilesDirectory, NewDirPath);
    
    char NewWildcard[16];
    if(Wildcard){
        CopyStrings(NewWildcard, Wildcard);
    }
    else{
        CopyStrings(NewWildcard, "*");
    }
    
    char ActualFindString[MAX_PATH];
    ConcatStringsUnsafe(ActualFindString, NewDirPath, NewWildcard);
    
    size_t MemNeeded = 0;
    
    GlobalWin32.OpenFilesFindHandle =  FindFirstFileA(
        ActualFindString,
        &GlobalWin32.OpenFilesFindData);
    
    GlobalWin32.OpenFilesNextFound = GlobalWin32.OpenFilesFindHandle != INVALID_HANDLE_VALUE;
}

PLATFORM_OPEN_FILES_END(Win32OpenFilesEnd){
    GlobalWin32.OpenFilesDirectory[0] = 0;
    
    FindClose(GlobalWin32.OpenFilesFindHandle);
    
    ASSERT(GlobalWin32.InOpenFilesBlock == 1);
    GlobalWin32.InOpenFilesBlock = 0;
}

PLATFORM_OPEN_NEXT_FILE(Win32OpenNextFile){
    b32 Result = GlobalWin32.OpenFilesNextFound;
    
    platform_file_desc ResultDesc = {};
    
    if(Result){
        WIN32_FIND_DATAA* FindData = &GlobalWin32.OpenFilesFindData;
        
        // NOTE(Dima): Copy file name
        CopyStrings(ResultDesc.Name, FindData->cFileName);
        
        // NOTE(Dima): Full file name
        ConcatStringsUnsafe(ResultDesc.FullPath, 
                            GlobalWin32.OpenFilesDirectory,
                            FindData->cFileName);
        
        // NOTE(Dima): Getting size
        ResultDesc.Size = (((u64)FindData->nFileSizeHigh) << 32) | (FindData->nFileSizeLow);
        
        // NOTE(Dima): Setting various flags
        DWORD Attrs = FindData->dwFileAttributes;
        u32 *Flags = &ResultDesc.Flags;
        if(Attrs & FILE_ATTRIBUTE_ARCHIVE){
            *Flags |= File_Archive;
        }
        
        if(Attrs & FILE_ATTRIBUTE_COMPRESSED){
            *Flags |= File_Compressed;
        }
        
        if(Attrs & FILE_ATTRIBUTE_DIRECTORY){
            *Flags |= File_Directory;
        }
        
        if(Attrs & FILE_ATTRIBUTE_HIDDEN){
            *Flags |= File_Hidden;
        }
        
        if(Attrs & FILE_ATTRIBUTE_NORMAL){
            *Flags |= File_Normal;
        }
        
        if(Attrs & FILE_ATTRIBUTE_READONLY){
            *Flags |= File_Readonly;
        }
        
        if(Attrs & FILE_ATTRIBUTE_SYSTEM){
            *Flags |= File_System;
        }
        
        GlobalWin32.OpenFilesNextFound = FindNextFileA(GlobalWin32.OpenFilesFindHandle, 
                                                       &GlobalWin32.OpenFilesFindData);
    }
    
    if(OutFile){
        *OutFile = ResultDesc;
    }
    
    return(Result);
}

PLATFORM_FILE_OFFSET_READ(Win32FileOffsetRead){
    b32 Result = 0;
    
    HANDLE FileHandle = CreateFileA(
        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);
    
    if(FileHandle != INVALID_HANDLE_VALUE){
        
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = Offset & 0xFFFFFFFF;
        Overlapped.OffsetHigh = (Offset >> 32) & 0xFFFFFFFF;
        
        DWORD BytesRead;
        BOOL ReadSuccess = ReadFile(
            FileHandle,
            ReadTo,
            (u32)ReadCount,
            &BytesRead,
            &Overlapped);
        
        Result = ReadSuccess && (BytesRead == ReadCount);
    }
    
    return(Result);
}

RENDER_PLATFORM_SWAPBUFFERS(Win32OpenGLSwapBuffers){
    SwapBuffers(GlobalWin32.glDC);
}

RENDER_PLATFORM_INIT(Win32OpenGLRenderInit){
    GlobalWin32.glDC = GetDC(GlobalWin32.window);
    GlobalWin32.renderCtx = Win32InitOpenGL(GlobalWin32.glDC);
    
#if JOY_USE_OPENGL
    GlInit(&GlobalGL, Assets);
#endif
    
#if JOY_USE_DIRECTX
    DirXInit(&GlobalDirX, 
             win32.window, 
             win32.windowWidth,
             win32.windowHeight);
#endif
}

RENDER_PLATFORM_FREE(Win32OpenGLRenderFree){
    //NOTE(dima): Cleanup
    GlFree(&GlobalGL);
    
#if JOY_USE_DIRECTX
    DirXFree(&GlobalDirX);
#endif
#if JOY_USE_OPENGL
    Win32FreeOpenGL(GlobalWin32.renderCtx);
#endif
    
    ReleaseDC(GlobalWin32.window, GlobalWin32.glDC);
}

RENDER_PLATFORM_RENDER(Win32OpenGLRender){
    GlOutputRender(&GlobalGL, GlobalGame->Render);
}

INTERNAL_FUNCTION void
Win32ToggleFullscreen(win_state* win32)
{
    DWORD style = GetWindowLong(win32->window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monInfo = { sizeof(monInfo) };
        if (GetWindowPlacement(win32->window, &win32->windowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(win32->window, MONITOR_DEFAULTTOPRIMARY), &monInfo))
        {
            SetWindowLong(win32->window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(win32->window, HWND_TOP,
                         monInfo.rcMonitor.left, monInfo.rcMonitor.top,
                         monInfo.rcMonitor.right - monInfo.rcMonitor.left,
                         monInfo.rcMonitor.bottom - monInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(win32->window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(win32->window, &win32->windowPlacement);
        SetWindowPos(win32->window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

inline void Win32ProcessKey(key_state* key, b32 IsDown, int RepeatCount){
    
    b32 ActualIsDown = IsDown != 0;
    
    if(key->EndedDown != ActualIsDown){
        key->EndedDown = ActualIsDown;
        
        key->TransitionHappened = 1;
    }
    key->RepeatCount = RepeatCount;
}

INTERNAL_FUNCTION void 
Win32ProcessMessages(input_state* Input){
    MSG msg;
    while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)){
        switch(msg.message){
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 vKey = (u32)msg.wParam;
                b32 wasDown = ((msg.lParam & (1 << 30)) != 0);
                b32 isDown = ((msg.lParam & (1 << 31)) == 0);
                b32 altKeyWasDown = ((msg.lParam & (1 << 29)) != 0);
                int RepeatCount = msg.lParam & 0xFFFF;
                
                //NOTE(dima): If state of key was changed
                u32 keyType = 0xFFFFFFFF;
                
                switch(vKey){
                    case VK_LBUTTON: { keyType = KeyMouse_Left; }break;
                    case VK_RBUTTON: { keyType = KeyMouse_Right; }break;
                    case VK_MBUTTON: { keyType = KeyMouse_Middle; }break;
                    case VK_XBUTTON1: { keyType = KeyMouse_X1; }break;
                    case VK_XBUTTON2: { keyType = KeyMouse_X2; }break;
                    case VK_LEFT: { keyType = Key_Left; }break;
                    case VK_RIGHT: { keyType = Key_Right; }break;
                    case VK_UP: { keyType = Key_Up; }break;
                    case VK_DOWN: { keyType = Key_Down; }break;
                    case VK_BACK: { keyType = Key_Backspace; }break;
                    case VK_TAB: { keyType = Key_Tab; }break;
                    case VK_RETURN: { keyType = Key_Return; }break;
                    case VK_SHIFT: { keyType = Key_Shift; }break;
                    case VK_CONTROL: { keyType = Key_Control; }break;
                    case VK_ESCAPE: { keyType= Key_Escape; }break;
                    case VK_SPACE: { keyType = Key_Space; }break;
                    case VK_HOME: { keyType = Key_Home; }break;
                    case VK_END: { keyType = Key_End; }break;
                    case VK_INSERT: { keyType = Key_Insert; }break;
                    case VK_DELETE: { keyType = Key_Delete; }break;
                    case VK_HELP: { keyType = Key_Help; }break;
                    
                    case 0x30:{ keyType = Key_0; }break;
                    case 0x31:{ keyType = Key_1; }break;
                    case 0x32:{ keyType = Key_2; }break;
                    case 0x33:{ keyType = Key_3; }break;
                    case 0x34:{ keyType = Key_4; }break;
                    case 0x35:{ keyType = Key_5; }break;
                    case 0x36:{ keyType = Key_6; }break;
                    case 0x37:{ keyType = Key_7; }break;
                    case 0x38:{ keyType = Key_8; }break;
                    case 0x39:{ keyType = Key_9; }
                    
                    case 'A':{ keyType = Key_A; }break;
                    case 'B':{ keyType = Key_B; }break;
                    case 'C':{ keyType = Key_C; }break;
                    case 'D':{ keyType = Key_D; }break;
                    case 'E':{ keyType = Key_E; }break;
                    case 'F':{ keyType = Key_F; }break;
                    case 'G':{ keyType = Key_G; }break;
                    case 'H':{ keyType = Key_H; }break;
                    case 'I':{ keyType = Key_I; }break;
                    case 'J':{ keyType = Key_J; }break;
                    case 'K':{ keyType = Key_K; }break;
                    case 'L':{ keyType = Key_L; }break;
                    case 'M':{ keyType = Key_M; }break;
                    case 'N':{ keyType = Key_N; }break;
                    case 'O':{ keyType = Key_O; }break;
                    case 'P':{ keyType = Key_P; }break;
                    case 'Q':{ keyType = Key_Q; }break;
                    case 'R':{ keyType = Key_R; }break;
                    case 'S':{ keyType = Key_S; }break;
                    case 'T':{ keyType = Key_T; }break;
                    case 'U':{ keyType = Key_U; }break;
                    case 'V':{ keyType = Key_V; }break;
                    case 'W':{ keyType = Key_W; }break;
                    case 'X':{ keyType = Key_X; }break;
                    case 'Y':{ keyType = Key_Y; }break;
                    case 'Z':{ keyType = Key_Z; }break;
                    
                    case VK_NUMPAD0: { keyType = Key_Num0; }break;
                    case VK_NUMPAD1: { keyType = Key_Num1; }break;
                    case VK_NUMPAD2: { keyType = Key_Num2; }break;
                    case VK_NUMPAD3: { keyType = Key_Num3; }break;
                    case VK_NUMPAD4: { keyType = Key_Num4; }break;
                    case VK_NUMPAD5: { keyType = Key_Num5; }break;
                    case VK_NUMPAD6: { keyType = Key_Num6; }break;
                    case VK_NUMPAD7: { keyType = Key_Num7; }break;
                    case VK_NUMPAD8: { keyType = Key_Num8; }break;
                    case VK_NUMPAD9: { keyType = Key_Num9; }break;
                    case VK_MULTIPLY: { keyType = Key_Multiply; }break;
                    case VK_ADD: { keyType = Key_Add; }break;
                    case VK_DIVIDE: { keyType = Key_Divide; }break;
                    case VK_SUBTRACT: { keyType = Key_Subtract; }break;
                    case VK_SEPARATOR: { keyType = Key_Separator; }break;
                    case VK_DECIMAL: { keyType = Key_Decimal; }break;
                    case VK_F1: {  keyType = Key_F1; }break;
                    case VK_F2: {  keyType = Key_F2; }break;
                    case VK_F3: {  keyType = Key_F3; }break;
                    case VK_F4: {  keyType = Key_F4; }break;
                    case VK_F5: {  keyType = Key_F5; }break;
                    case VK_F6: {  keyType = Key_F6; }break;
                    case VK_F7: {  keyType = Key_F7; }break;
                    case VK_F8: {  keyType = Key_F8; }break;
                    case VK_F9: {  keyType = Key_F9; }break;
                    case VK_F10: {  keyType = Key_F10; }break;
                    case VK_F11: {  keyType = Key_F11; }break;
                    case VK_F12: {  keyType = Key_F12; }break;
                    case VK_VOLUME_MUTE: { keyType = Key_VolumeMute; }break;
                    case VK_VOLUME_UP: { keyType = Key_VolumeUp; }break;
                    case VK_VOLUME_DOWN: { keyType = Key_VolumeDown; }break;
                    case VK_OEM_3:{keyType = Key_OEM3;}break;
                    default: {}break;
                }
                
                if(wasDown != isDown){
                    
                    if(isDown){
                        
                        if(altKeyWasDown && vKey == VK_F4){
                            GlobalRunning = 0;
                        }
                        
                        if(altKeyWasDown && vKey == VK_RETURN){
                            Win32ToggleFullscreen(&GlobalWin32);
                        }
                    }
                }
                
                if(keyType != 0xFFFFFFFF){
                    Win32ProcessKey(&Input->Keyboard.KeyStates[keyType], isDown, RepeatCount);
                }
                
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }break;
            
            case WM_LBUTTONDOWN:{
                
            }break;
            
            case WM_QUIT:{
                GlobalRunning = 0;
            }break;
            
            case WM_CLOSE:{
                PostQuitMessage(0);
                GlobalRunning = 0;
            }break;
            
            case WM_DESTROY:{
                PostQuitMessage(0);
                GlobalRunning = 0;
            }break;
            
            default:{
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }break;
        } //END SWITCH
    } //END_WHILE
}

INTERNAL_FUNCTION void
Win32PreProcessInput(input_state* Input){
    // NOTE(Dima): Char input
    Input->FrameInput[0] = 0;
    Input->FrameInputLen = 0;
    
    for(int keyIndex = 0; keyIndex < Key_Count; keyIndex++){
        Input->Keyboard.KeyStates[keyIndex].TransitionHappened = 0;
        Input->Keyboard.KeyStates[keyIndex].RepeatCount = 0;
    }
    
#if 0    
    for(int PadIndex = 0; PadIndex < MAX_GAMEPAD_COUNT; PadIndex++){
        gamepad_controller* Pad = &Input->GamepadControllers[PadIndex];
        
        for(int KeyIndex = 0; KeyIndex < GamepadKey_Count; KeyIndex++){
            key_state* Key = &Pad->Keys[KeyIndex].Key;
            
            Key->TransitionHappened = 0;
            Key->RepeatCount = 0;
        }
    }
#endif
    
}

INTERNAL_FUNCTION inline rc2 Win32RectToJoy(RECT Rect){
    rc2 Result = {};
    
    Result.min.x = Rect.left;
    Result.min.y = Rect.top;
    Result.max.x = Rect.right;
    Result.max.y = Rect.bottom;
    
    return(Result);
}

INTERNAL_FUNCTION void Win32XInputProcessStick(gamepad_stick* Stick, 
                                               float LX, float LY,
                                               int Deadzone)
{
    v2 Unnorm = V2(LX, LY);
    
    float Mag = Magnitude(Unnorm);
    
    v2 Norm = V2(LX / Mag, LY / Mag);
    
    float NormalizedMagnitude = 0.0f;
    
    if(Mag > Deadzone){
        // NOTE(Dima): Clip
        Mag = Min(Mag, 32767);
        
        // NOTE(Dima): Adjusting magnitude to the input of the deadzone
        Mag -= Deadzone;
        
        Stick->Magnitude = Mag / (32767 - Deadzone);
        Stick->Direction = Norm;
    }
    else {
        Stick->Magnitude = 0.0f;
        Stick->Direction = V2(0.0f, 0.0f);
    }
}

INTERNAL_FUNCTION void
Win32ProcessInput(input_state* Input)
{
    Input->Time = Time;
    Input->DeltaTime = DeltaTime;
    
    // NOTE(Dima): Getting mouse posititons
    POINT point;
    GetCursorPos(&point);
    v2 MouseInScreenP = V2(point.x, point.y);
    
    // NOTE(Dima): Getting current mouse P in-window
    ScreenToClient(GlobalWin32.window, &point);
    v2 CurMouseP = V2(point.x, point.y);
    
    v2 MouseActualDelta = {};
    if(Input->NotFirstFrame){
        MouseActualDelta = CurMouseP - Input->MouseP;
    }
    else{
        Input->NotFirstFrame = true;
    }
    
    // NOTE(Dima): Processing capturing mouse
    
    if(Input->CapturingMouse){
        HMONITOR MonitorHandle = MonitorFromWindow(
            GlobalWin32.window, 
            MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO MonitorInfo;
        MonitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoA(MonitorHandle, &MonitorInfo);
        
        rc2 MonitorRect = Win32RectToJoy(MonitorInfo.rcWork);
        v2 MonitorDim = GetRectDim(MonitorRect);
        
        float OverlapBorderWidth = 3.0f;
        
        rc2 MouseCanMoveRect = GrowRectByPixels(MonitorRect, -OverlapBorderWidth);
        
        v2 ChangedP = MouseInScreenP;
        
        if(ChangedP.x < MouseCanMoveRect.min.x){
            ChangedP.x = MouseCanMoveRect.max.x;
        }
        if(ChangedP.y < MouseCanMoveRect.min.y){
            ChangedP.y = MouseCanMoveRect.max.y;
        }
        if(ChangedP.x > MouseCanMoveRect.max.x){
            ChangedP.x = MouseCanMoveRect.min.x;
        }
        if(ChangedP.y > MouseCanMoveRect.max.y){
            ChangedP.y = MouseCanMoveRect.min.y;
        }
        
        point.x = ChangedP.x;
        point.y = ChangedP.y;
        SetCursorPos(point.x, point.y);
    }
    else{
        point.x = MouseInScreenP.x;
        point.y = MouseInScreenP.y;
        SetCursorPos(point.x, point.y);
    }
    
    ScreenToClient(GlobalWin32.window, &point);
    v2 MouseP = V2(point.x, point.y);
    
    Input->MouseP = MouseP;
    
    Input->MouseDeltaPActual = MouseActualDelta;
    Input->MouseDeltaP = Input->MouseDeltaPActual;
    //Input->MouseDeltaP.y = -Input->MouseDeltaP.y;
    Input->MouseDeltaP = -Input->MouseDeltaP;
    
    // NOTE(Dima): Processing gamepads
    for(int ControllerIndex = 0;
        ControllerIndex < XUSER_MAX_COUNT;
        ControllerIndex++)
    {
        ASSERT(ControllerIndex < ARRAY_COUNT(Input->GamepadControllers));
        gamepad_controller* Controller = &Input->GamepadControllers[ControllerIndex];
        
        XINPUT_STATE State;
        DWORD GetControlRes = XInputGetState(ControllerIndex,
                                             &State);
        
        if(GetControlRes == ERROR_SUCCESS){
            // NOTE(Dima): Controller is connected
            XINPUT_GAMEPAD* XPad = &State.Gamepad;
            Controller->IsConnected = true;
            
            // NOTE(Dima): Battery info getting
            XINPUT_BATTERY_INFORMATION BatteryInfo;
            DWORD GetBatteryRes = XInputGetBatteryInformation(
                ControllerIndex,
                BATTERY_DEVTYPE_GAMEPAD,
                &BatteryInfo);
            
            if(GetBatteryRes == ERROR_SUCCESS){
                switch(BatteryInfo.BatteryLevel){
                    case BATTERY_LEVEL_EMPTY:{
                        Controller->BatteryCharge = GamepadBattery_Empty;
                    }break;
                    
                    case BATTERY_LEVEL_LOW:{
                        Controller->BatteryCharge = GamepadBattery_Low;
                    }break;
                    
                    case BATTERY_LEVEL_MEDIUM:{
                        Controller->BatteryCharge = GamepadBattery_Medium;
                    }break;
                    
                    case BATTERY_LEVEL_FULL:{
                        Controller->BatteryCharge = GamepadBattery_Full;
                    }break;
                    
                    default:{
                        Controller->BatteryCharge = GamepadBattery_Empty;
                    }break;
                }
            }
            
            // NOTE(Dima): Processing sticks
            Win32XInputProcessStick(&Controller->LeftStick, 
                                    XPad->sThumbLX, XPad->sThumbLY,
                                    XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            Win32XInputProcessStick(&Controller->RightStick, 
                                    XPad->sThumbRX, XPad->sThumbRY,
                                    XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            
            XINPUT_KEYSTROKE Keystroke;
            while(XInputGetKeystroke(ControllerIndex, 0, &Keystroke) != ERROR_EMPTY){
                int KeyToProcess = -1;
                
                switch(Keystroke.VirtualKey){
                    case VK_PAD_A:{KeyToProcess = GamepadKey_A;}break;
                    case VK_PAD_B:{KeyToProcess = GamepadKey_B;}break;
                    case VK_PAD_X:{KeyToProcess = GamepadKey_X;}break;
                    case VK_PAD_Y:{KeyToProcess = GamepadKey_Y;}break;
                    case VK_PAD_RSHOULDER:{KeyToProcess = GamepadKey_RightShoulder;}break;
                    case VK_PAD_LSHOULDER:{KeyToProcess = GamepadKey_LeftShoulder;}break;
                    case VK_PAD_LTRIGGER:{KeyToProcess = GamepadKey_LeftTrigger;}break;
                    case VK_PAD_RTRIGGER:{KeyToProcess = GamepadKey_RightTrigger;}break;
                    case VK_PAD_DPAD_UP:{KeyToProcess = GamepadKey_DpadUp;}break;
                    case VK_PAD_DPAD_DOWN:{KeyToProcess = GamepadKey_DpadDown;}break;
                    case VK_PAD_DPAD_LEFT:{KeyToProcess = GamepadKey_DpadLeft;}break;
                    case VK_PAD_DPAD_RIGHT:{KeyToProcess = GamepadKey_DpadRight;}break;
                    case VK_PAD_START:{KeyToProcess = GamepadKey_Start;}break;
                    case VK_PAD_BACK:{KeyToProcess = GamepadKey_Back;}break;
                    case VK_PAD_LTHUMB_PRESS:{KeyToProcess = GamepadKey_LeftThumb;}break;
                    case VK_PAD_RTHUMB_PRESS:{KeyToProcess = GamepadKey_RightThumb;}break;
                }
                
                b32 IsDown = (Keystroke.Flags & (XINPUT_KEYSTROKE_KEYDOWN | 
                                                 XINPUT_KEYSTROKE_REPEAT)) != 0;
                
                if(KeyToProcess != -1){
                    Win32ProcessKey(&Controller->Keys[KeyToProcess].Key, IsDown, 0);
                }
            }
        }
        else {
            // NOTE(Dima): Controller is not connected
            Controller->IsConnected = false;
        }
    }
    
    //NOTE(Dima): Processing mouse buttons
    DWORD Win32MouseKeyID[] = {
        VK_LBUTTON,
        VK_MBUTTON,
        VK_RBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };
    
    for(u32 mouseKeyIndex = 0;
        mouseKeyIndex < ARRAY_COUNT(Win32MouseKeyID);
        mouseKeyIndex++)
    {
        Input->Keyboard.KeyStates[MouseKey_Left + mouseKeyIndex].TransitionHappened = 0;
        SHORT winMouseKeyState = GetKeyState(Win32MouseKeyID[mouseKeyIndex]);
        
        Win32ProcessKey(&Input->Keyboard.KeyStates[MouseKey_Left + mouseKeyIndex], winMouseKeyState & (1 << 15), 0);
    }
    
    // NOTE(Dima): Processing input structure buttons and controllers
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        input_controller* Cont = &Input->Controllers[ControllerIndex];
        
        for(int ButIndex = 0; ButIndex < Button_Count; ButIndex++){
            button_state* But = &Cont->Buttons[ButIndex];
            
            key_state* ActiveKey = 0;
            
            for(int ButKeyIndex = 0; 
                ButKeyIndex < But->KeyCount; 
                ButKeyIndex++)
            {
                key_state* CorrespondingKey = 0;
                
                u32 KeyIndex = But->Keys[ButKeyIndex];
                
                switch(Cont->ControllerSource){
                    case InputControllerSource_Keyboard:{
                        CorrespondingKey = &Input->Keyboard.KeyStates[KeyIndex];
                    }break;
                    case InputControllerSource_Gamepad:{
                        CorrespondingKey =
                            &Input->GamepadControllers[Cont->GamepadIndex].Keys[KeyIndex].Key;
                    }break;
                }
                
                if(CorrespondingKey){
                    if(!ActiveKey){
                        ActiveKey = CorrespondingKey;
                    }
                    
                    if(CorrespondingKey->TransitionHappened){
                        ActiveKey = CorrespondingKey;
                        
                        break;
                    }
                }
            }
            
            if(But->KeyCount && ActiveKey){
                
                But->EndedDown = ActiveKey->EndedDown;
                But->TransitionHappened = ActiveKey->TransitionHappened;
                But->InTransitionTime += DeltaTime;
                
                if(But->TransitionHappened){
                    But->InTransitionTime = 0.0f;
                }
            }
        }
    }
}

INPUT_PLATFORM_PROCESS(Win32PlatformInputProcess){
    Win32PreProcessInput(GlobalGame->Input);
    Win32ProcessMessages(GlobalGame->Input);
    Win32ProcessInput(GlobalGame->Input);
}

INTERNAL_FUNCTION u32 UTF8_Sequence_Size_For_UCS4(u32 u)
{
    // Returns number of bytes required to encode 'u'
    static const u32 CharBounds[] = { 
        0x0000007F, 
        0x000007FF, 
        0x0000FFFF, 
        0x001FFFFF, 
        0x03FFFFFF, 
        0x7FFFFFFF, 
        0xFFFFFFFF };
    
    u32 bi = 0;
    while(CharBounds[bi] < u ){
        ++bi;
    }
    return bi+1;
}

// NOTE(Dima): Thanx Alex Podverbny (BLK Dragon) 4 the code
INTERNAL_FUNCTION u32 UTF16_To_UTF8(u16* UTF16String, 
                                    u8* To, u32 ToLen, 
                                    u32* OutToSize)
{
    u8*         s       = To;
    u8*         s_end   = To + ToLen;
    u16*  w       = UTF16String;
    u32        len     = 0;
    while(*w)
    {
        u32    ch = *w ;
        u32    sz = UTF8_Sequence_Size_For_UCS4( ch );
        if( s + sz >= s_end )
            break;
        if(sz == 1)
        {
            // just one byte, no header
            *s = (u8)(ch);
            ++s;
        }
        else
        {
            // write the bits 6 bits at a time, 
            // except for the first one, which can be less than 6 bits
            u32 shift = (sz-1) * 6;
            *s = uint8_t(((ch >> shift) & 0x3F) | (0xFF << (8 - sz)));
            shift -= 6;
            ++s;
            for(u32 i=1; i!=sz; ++i,shift-=6 )
            {
                *s = u8(((ch >> shift) & 0x3F) | 0x80);
                ++s;
            }
        }
        ++len;
        ++w;
    }
    
    *s = 0x00;
    if(OutToSize){
        *OutToSize = (s - To);
    }
    
    return len;
}

WIN32_DEBUG_OUTPUT(Win32DebugOutputLog){
    HANDLE HandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD CharsWritten; // ignored
    DWORD Len = StringLength((char*)Str);
    WriteConsoleA(HandleOut, Str, Len, &CharsWritten, 0);
}


#if 0
static struct ODS_Buffer
{
    DWORD process_id;
    char  data[4096 - sizeof(DWORD)];
}* ods_buffer;

static HANDLE ods_data_ready;
static HANDLE ods_buffer_ready;

INTERNAL_FUNCTION DWORD WINAPI OutputDebugString_Proc(LPVOID arg)
{
    DWORD ret = 0;
    
    HANDLE StdERR = GetStdHandle(STD_ERROR_HANDLE);
    Assert(StdERR);
    
    for (;;)
    {
        SetEvent(ods_buffer_ready);
        
        DWORD wait = WaitForSingleObject(ods_data_ready, INFINITE);
        Assert(wait == WAIT_OBJECT_0);
        
        // NOTE(Dima): Getting the str length
        DWORD length = 0;
        while (length < sizeof(ods_buffer->data) && ods_buffer->data[length] != 0)
        {
            length++;
        }
        
        // NOTE(Dima): Write to StdERR
        if (length != 0)
        {
            DWORD written;
            WriteFile(StdERR, ods_buffer->data, length, &written, NULL);
        }
    }
}

INTERNAL_FUNCTION void OutputDebugString_Capture()
{
    if (IsDebuggerPresent())
    {
        return;
    }
    
    HANDLE file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*ods_buffer), "DBWIN_BUFFER");
    Assert(file != INVALID_HANDLE_VALUE);
    
    ods_buffer = (ODS_Buffer*)MapViewOfFile(file, SECTION_MAP_READ, 0, 0, 0);
    Assert(ods_buffer);
    
    ods_buffer_ready = CreateEventA(NULL, FALSE, FALSE, "DBWIN_BUFFER_READY");
    Assert(ods_buffer_ready);
    
    ods_data_ready = CreateEventA(NULL, FALSE, FALSE, "DBWIN_DATA_READY");
    Assert(ods_data_ready);
    
    HANDLE thread = CreateThread(NULL, 0, OutputDebugString_Proc, NULL, 0, NULL);
    Assert(thread);
}
#endif

LRESULT CALLBACK
Win32WindowProcessing(
HWND Window,
UINT Message,
WPARAM WParam,
LPARAM LParam)
{
    switch (Message){
        
        case WM_SIZE:{
            
        }break;
        
        case WM_CHAR:{
            input_state* Input = GlobalGame->Input;
            Input->FrameInput[Input->FrameInputLen++] = (char)WParam;
            Input->FrameInput[Input->FrameInputLen] = 0;
        }break;
        
        case WM_DESTROY:{
            GlobalRunning = 0;
        }break;
        
        case WM_QUIT:{
            GlobalRunning = 0;
        }break;
        
        case WM_CLOSE:{
            GlobalRunning = 0;
        }break;
        
        default:{
            return DefWindowProc(Window, Message, WParam, LParam);
        }break;
    }
    
    return(0);
}

INTERNAL_FUNCTION void Win32InitWindow(HINSTANCE Instance,
                                       int WindowWidth, 
                                       int WindowHeight)
{
    //WindowWidth = (WindowWidth + 3) & (~3);
    
    int WindowCreateW;
    int WindowCreateH;
    
    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.lpszClassName = "MainWindowClassName";
    wndClass.hInstance = Instance;
    wndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = Win32WindowProcessing;
    
    RegisterClassExA(&wndClass);
    
    RECT ClientRect;
    ClientRect.left = 0;
    ClientRect.top = 0;
    ClientRect.right = WindowWidth;
    ClientRect.bottom = WindowHeight;
    BOOL WindowRectAdjusted = AdjustWindowRect(
        &ClientRect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & (~WS_OVERLAPPED), 0);
    
    if(WindowRectAdjusted){
        WindowCreateW = ClientRect.right - ClientRect.left;
        WindowCreateH = ClientRect.bottom - ClientRect.top;
    }
    else{
        WindowCreateW = WindowWidth;
        WindowCreateH = WindowHeight;
    }
    
    GlobalWin32.window = CreateWindowA(
        wndClass.lpszClassName,
        "Joy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowCreateW,
        WindowCreateH,
        0, 0, 
        Instance, 
        0);
    
    GlobalWin32.WindowWidth = WindowWidth;
    GlobalWin32.WindowHeight = WindowHeight;
    
    void* winBmpMemory = PushSomeMem(&GlobalMem, WindowWidth * WindowHeight * 4, 64);
    GlobalWin32.bitmap = AllocateBitmapInternal(WindowWidth, WindowHeight, winBmpMemory);
    GlobalWin32.bmi = {};
    BITMAPINFO* bmi = &GlobalWin32.bmi;
    BITMAPINFOHEADER* bmiHeader = &bmi->bmiHeader;
    bmiHeader->biSize=  sizeof(BITMAPINFOHEADER);
    bmiHeader->biWidth = WindowWidth;
    bmiHeader->biHeight = -WindowHeight;
    bmiHeader->biPlanes = 1;
    bmiHeader->biBitCount = 32;
    bmiHeader->biCompression = BI_RGB;
    bmiHeader->biSizeImage = 0;
    
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    //NOTE(Dima): Setting critical sections
    for(int i = 0; i < MAX_CRITICAL_SECTIONS_COUNT; i++){
        GlobalWin32.NotUsedCriticalSectionIndices[i] = i;
    }
    GlobalWin32.NotUsedCriticalSectionIndicesCount = MAX_CRITICAL_SECTIONS_COUNT;
    
    // NOTE(Dima): Initializing platform API
    InitJobQueue(&Platform.highPriorityQueue, 2048, 8);
    InitJobQueue(&Platform.lowPriorityQueue, 2048, 4);
    
    Platform.AddEntry = PlatformAddEntry;
    Platform.WaitForCompletion = PlatformWaitForCompletion;
    Platform.ReadFile = Win32ReadFile;
    Platform.WriteFile = Win32WriteFile;
    Platform.FreeFileMemory = Win32FreeFileMemory;
    Platform.ShowError = Win32ShowError;
    Platform.OutputString = Win32DebugOutputString;
    Platform.MemAlloc = Win32MemAlloc;
    Platform.MemFree = Win32MemFree;
    Platform.MemZero = Win32MemZero;
    Platform.MemZeroRaw = Win32MemZeroRaw;
    Platform.OpenFilesBegin = Win32OpenFilesBegin;
    Platform.OpenFilesEnd = Win32OpenFilesEnd;
    Platform.OpenNextFile = Win32OpenNextFile;
    Platform.FileOffsetRead = Win32FileOffsetRead;
    Platform.InitMutex = Win32InitMutex;
    Platform.LockMutex = Win32LockMutex;
    Platform.UnlockMutex = Win32UnlockMutex;
    Platform.FreeMutex = Win32FreeMutex;
    
    // NOTE(Dima): Initializing memory sentinel
    GlobalWin32.memorySentinel = {};
    GlobalWin32.memorySentinel.Prev = &GlobalWin32.memorySentinel;
    GlobalWin32.memorySentinel.Next = &GlobalWin32.memorySentinel;
    
    // TODO(Dima): Add array of count Renderer_Count and init all renderers
    // TODO(Dima): Or leave if not supported
    // NOTE(Dima): Init render API
    render_platform_api RenderPlatformAPI = {};
    RenderPlatformAPI.RendererType = Renderer_OpenGL;
    RenderPlatformAPI.SwapBuffers = Win32OpenGLSwapBuffers;
    RenderPlatformAPI.Init = Win32OpenGLRenderInit;
    RenderPlatformAPI.Free = Win32OpenGLRenderFree;
    RenderPlatformAPI.Render = Win32OpenGLRender;
    
    // NOTE(Dima): Initializing platfor to game API
    platform_to_game_api Platform2Game = {};
    Platform2Game.RenderAPI = RenderPlatformAPI;
    Platform2Game.ProcessInput = Win32PlatformInputProcess;
    
    // NOTE(Dima): Calculating perfomance frequency
    QueryPerformanceFrequency(&GlobalWin32.PerformanceFreqLI);
    GlobalWin32.OneOverPerformanceFreq = 1.0 / (double)GlobalWin32.PerformanceFreqLI.QuadPart;
    
    platform_mutex Mutex1;
    platform_mutex Mutex2;
    platform_mutex Mutex3;
    
    Win32InitMutex(&Mutex1);
    Win32InitMutex(&Mutex2);
    Win32InitMutex(&Mutex3);
    
    Win32FreeMutex(&Mutex1);
    Win32FreeMutex(&Mutex3);
    Win32FreeMutex(&Mutex2);
    
    // NOTE(Dima): Init win32 debug output log func
    if(IsDebuggerPresent()){
        GlobalWin32.DebugOutputFunc = OutputDebugStringA;
    }
    else{
        //AllocConsole();
        GlobalWin32.DebugOutputFunc = Win32DebugOutputLog;
        
        // redirect unbuffered STDOUT to the console
        intptr_t stdHandle = reinterpret_cast<intptr_t>(::GetStdHandle(STD_OUTPUT_HANDLE));
        int	conHandle = _open_osfhandle(stdHandle, _O_TEXT);
        FILE* file = _fdopen(conHandle, "w");
        *stdout = *file;
        setvbuf(stdout, NULL, _IONBF, 0);
        
        // redirect unbuffered STDIN to the console
        stdHandle = reinterpret_cast<intptr_t>(::GetStdHandle(STD_INPUT_HANDLE));
        conHandle = _open_osfhandle(stdHandle, _O_TEXT);
        file = _fdopen(conHandle, "r");
        *stdin = *file;
        setvbuf(stdin, NULL, _IONBF, 0);
        
        // redirect unbuffered STDERR to the console
        stdHandle = reinterpret_cast<intptr_t>(::GetStdHandle(STD_ERROR_HANDLE));
        conHandle = _open_osfhandle(stdHandle, _O_TEXT);
        file = _fdopen(conHandle, "w");
        *stderr = *file;
        setvbuf(stderr, NULL, _IONBF, 0);
        
        //GlobalWin32.DebugOutputFunc = OutputDebugStringA;
        //OutputDebugString_Capture();
    }
    
    int WindowWidth = 1366;
    int WindowHeight = 768;
    Win32InitWindow(hInstance, WindowWidth, WindowHeight);
    
    GlobalGame = PushStruct(&GlobalMem, game_state);
    GameInit(GlobalGame, Platform2Game);
    
    // NOTE(Dima): Xinput Init
    for(int ControllerIndex = 0;
        ControllerIndex < XUSER_MAX_COUNT;
        ControllerIndex++)
    {
        ZeroMemory(&GlobalWin32.ControllerStates[ControllerIndex], sizeof(XINPUT_STATE));
    }
    
    // NOTE(Dima): DSound init
    DSoundInit(&GlobalDirectSound, GlobalWin32.window);
    Win32ClearSoundBuffer(&GlobalDirectSound);
    //Win32FillSoundBufferWithSound(&GlobalDirectSound, &gAssets.SineTest1);
    Win32PlayDirectSoundBuffer(&GlobalDirectSound);
    
    //ShellExecuteA(NULL, "open", "http://www.microsoft.com", NULL, NULL, SW_SHOWNORMAL);
    
    LARGE_INTEGER BeginClockLI;
    QueryPerformanceCounter(&BeginClockLI);
    
    GlobalRunning = 1;
    while(GlobalRunning){
        // NOTE(Dima): Processing time
        LARGE_INTEGER LastFrameClockLI;
        QueryPerformanceCounter(&LastFrameClockLI);
        u64 ClocksElapsed4Frame = LastFrameClockLI.QuadPart - BeginClockLI.QuadPart;
        BeginClockLI.QuadPart = LastFrameClockLI.QuadPart;
        DeltaTime = (double)ClocksElapsed4Frame * GlobalWin32.OneOverPerformanceFreq;
        Time += DeltaTime;
        
        // NOTE(Dima): Setting Frame info to Pass to game
        render_frame_info FrameInfo = {};
        FrameInfo.Width = WindowWidth;
        FrameInfo.Height = WindowHeight;
        FrameInfo.SoftwareBuffer = &GlobalWin32.bitmap;
        FrameInfo.dt = DeltaTime;
        FrameInfo.RendererType = Renderer_OpenGL;
        
        GameUpdate(GlobalGame, FrameInfo);
    }
    
    GameFree(GlobalGame);
    
    DSoundFree(&GlobalDirectSound);
    
    DestroyWindow(GlobalWin32.window);
    
    FreeJobQueue(&Platform.highPriorityQueue);
    FreeJobQueue(&Platform.lowPriorityQueue);
    
    return (0);
}
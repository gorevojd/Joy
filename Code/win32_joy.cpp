#include "win32_joy.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

/*
TODO(Dima):
Assets:
Bake blur

Renderer:
Software 3d renderer

dima privet , kak dela? i tebia lybly
*/

GLOBAL_VARIABLE win32_state Win32;
GLOBAL_VARIABLE b32 GlobalRunning;
GLOBAL_VARIABLE input GlobalInput;
GLOBAL_VARIABLE assets GlobalAssets;
GLOBAL_VARIABLE gui_state GlobalGui;

platform_api PlatformAPI;

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
    
    HWND TempWND = CreateWindowExA(
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
    
    HDC TmpDC = GetDC(TempWND);
    
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
    int PixelFormat = ChoosePixelFormat(TmpDC, &pfd);
    DescribePixelFormat(TmpDC, PixelFormat, sizeof(pfd), &pfd);
    BOOL SetPixelFormatResult = SetPixelFormat(TmpDC, PixelFormat, &pfd);
    Assert(SetPixelFormatResult);
    
    HGLRC TmpRenderCtx = wglCreateContext(TmpDC);
    BOOL MakeCurrentResult = wglMakeCurrent(TmpDC, TmpRenderCtx);
    Assert(MakeCurrentResult);
    
    WGLGETFUN(wglChoosePixelFormatARB);
    WGLGETFUN(wglCreateContextAttribsARB);
    WGLGETFUN(wglGetExtensionStringARB);
    WGLGETFUN(wglGetExtensionStringEXT);
    WGLGETFUN(wglGetSwapIntervalEXT);
    WGLGETFUN(wglSwapIntervalEXT);
    
    wglMakeCurrent(TmpDC, 0);
    wglDeleteContext(TmpRenderCtx);
    ReleaseDC(TempWND, TmpDC);
    DestroyWindow(TempWND);
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
HGLRC Win32InitOpenGL(HDC RealDC){
    Win32LoadOpenglExtensions();
    
    const int PixelFormatAttribs[] = {
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
    
    int PixelFormat;
    UINT NumFormats = 0;
    wglChoosePixelFormatARB(RealDC, PixelFormatAttribs, 0, 1, &PixelFormat, &NumFormats);
    Assert(NumFormats);
    
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(RealDC, PixelFormat, sizeof(pfd), &pfd);
    BOOL SetPFResult = SetPixelFormat(RealDC, PixelFormat, &pfd);
    Assert(SetPFResult);
    
    // Specify that we want to create an OpenGL 3.3 core profile context
    const int Attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0,
    };
    
    HGLRC ResultContext = wglCreateContextAttribsARB(RealDC, 0, Attribs);
    Assert(ResultContext);
    
    BOOL MakeCurrentResult = wglMakeCurrent(RealDC, ResultContext);
    Assert(MakeCurrentResult);
    
    Win32GetOpenglFunctions();
    
    return(ResultContext);
}

void Win32FreeOpenGL(HGLRC RenderContext){
    wglDeleteContext(RenderContext);
}

PLATFORM_FREE_FILE_MEMORY(Win32FreeFileMemory){
    if (FileReadResult->Data != 0){
        VirtualFree(FileReadResult->Data, 0, MEM_RELEASE);
    }
    
    FileReadResult->Data = 0;
    FileReadResult->DataSize = 0;
}

PLATFORM_READ_FILE(Win32ReadFile){
    platform_read_file_result Res = {};
    
    HANDLE FileHandle = CreateFileA(
        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        0, 0);
    
    if (FileHandle != INVALID_HANDLE_VALUE){
        LARGE_INTEGER FileSizeLI;
        if (GetFileSizeEx(FileHandle, &FileSizeLI)){
            u32 FileSize = (FileSizeLI.QuadPart & 0xFFFFFFFF);
            Res.Data = VirtualAlloc(0, FileSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Res.Data){
                DWORD BytesRead;
                if (ReadFile(FileHandle, Res.Data, FileSize, &BytesRead, 0) && (FileSize == BytesRead)){
                    Res.DataSize = FileSize;
                }
                else{
                    Win32FreeFileMemory(&Res);
                }
            }
            else{
                //TODO(Dima): Logging. Can not allocate memory
            }
        }
        else{
            //TODO(Dima): Logging. Can't get file size
        }
        
        CloseHandle(FileHandle);
    }
    else{
        //TODO(Dima): Logging. Can't open file
    }
    
    return(Res);
}

PLATFORM_WRITE_FILE(Win32WriteFile){
    b32 Result = 0;
    
    HANDLE FileHandle = CreateFileA(FilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE){
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Data, Size, &BytesWritten, 0)){
            Result = (BytesWritten == Size);
        }
        else{
            //TODO(Dima): Logging
        }
        
        CloseHandle(FileHandle);
    }
    else{
        //TODO(Dima): Logging
    }
    
    
    return(Result);
}

PLATFORM_SHOW_ERROR(Win32ShowError){
    char* CaptionText = "Error";
    u32 MessageBoxType = MB_OK;
    
    switch(Type){
        case PlatformError_Error:{
            CaptionText = "Error";
            MessageBoxType |= MB_ICONERROR;
        }break;
        
        case PlatformError_Warning:{
            CaptionText = "Warning";
            MessageBoxType |= MB_ICONWARNING;
        }break;
        
        case PlatformError_Information:{
            CaptionText = "Information";
            MessageBoxType |= MB_ICONINFORMATION;
        }break;
    }
    
    MessageBoxA(0, Text, CaptionText, MessageBoxType);
}

PLATFORM_DEBUG_OUTPUT_STRING(Win32DebugOutputString){
    OutputDebugString(Text);
}

INTERNAL_FUNCTION void
Win32ToggleFullscreen(win32_state* Win32)
{
    DWORD Style = GetWindowLong(Win32->Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Win32->Window, &Win32->WindowPlacement) &&
            GetMonitorInfo(MonitorFromWindow(Win32->Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Win32->Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Win32->Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Win32->Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Win32->Window, &Win32->WindowPlacement);
        SetWindowPos(Win32->Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

inline void Win32ProcessKey(key_state* Key, b32 IsDown){
    if(Key->EndedDown != IsDown){
        Key->EndedDown = IsDown;
        
        Key->TransitionHappened = 1;
    }
}

INTERNAL_FUNCTION void 
Win32ProcessMessages(input* Input){
    MSG Msg;
    while(PeekMessageA(&Msg, 0, 0, 0, PM_REMOVE)){
        switch(Msg.message){
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKey = (u32)Msg.wParam;
                b32 WasDown = ((Msg.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Msg.lParam & (1 << 31)) == 0);
                b32 AltKeyWasDown = ((Msg.lParam & (1 << 29)) != 0);
                
                //NOTE(dima): If state of key was changed
                if(WasDown != IsDown){
                    u32 KeyType;
                    switch(VKey){
                        case VK_LBUTTON: { KeyType = KeyMouse_Left; }break;
                        case VK_RBUTTON: { KeyType = KeyMouse_Right; }break;
                        case VK_MBUTTON: { KeyType = KeyMouse_Middle; }break;
                        case VK_XBUTTON1: { KeyType = KeyMouse_X1; }break;
                        case VK_XBUTTON2: { KeyType = KeyMouse_X2; }break;
                        case VK_LEFT: { KeyType = Key_Left; }break;
                        case VK_RIGHT: { KeyType = Key_Right; }break;
                        case VK_UP: { KeyType = Key_Up; }break;
                        case VK_DOWN: { KeyType = Key_Down; }break;
                        case VK_BACK: { KeyType = Key_Backspace; }break;
                        case VK_TAB: { KeyType = Key_Tab; }break;
                        case VK_RETURN: { KeyType = Key_Return; }break;
                        case VK_SHIFT: { KeyType = Key_Shift; }break;
                        case VK_CONTROL: { KeyType = Key_Control; }break;
                        case VK_ESCAPE: { KeyType= Key_Escape; }break;
                        case VK_SPACE: { KeyType = Key_Space; }break;
                        case VK_HOME: { KeyType = Key_Home; }break;
                        case VK_END: { KeyType = Key_End; }break;
                        case VK_INSERT: { KeyType = Key_Insert; }break;
                        case VK_DELETE: { KeyType = Key_Delete; }break;
                        case VK_HELP: { KeyType = Key_Help; }break;
                        
                        case 0x30:{ KeyType = Key_0; }break;
                        case 0x31:{ KeyType = Key_1; }break;
                        case 0x32:{ KeyType = Key_2; }break;
                        case 0x33:{ KeyType = Key_3; }break;
                        case 0x34:{ KeyType = Key_4; }break;
                        case 0x35:{ KeyType = Key_5; }break;
                        case 0x36:{ KeyType = Key_6; }break;
                        case 0x37:{ KeyType = Key_7; }break;
                        case 0x38:{ KeyType = Key_8; }break;
                        case 0x39:{ KeyType = Key_9; }
                        
                        case 'A':{ KeyType = Key_A; }break;
                        case 'B':{ KeyType = Key_B; }break;
                        case 'C':{ KeyType = Key_C; }break;
                        case 'D':{ KeyType = Key_D; }break;
                        case 'E':{ KeyType = Key_E; }break;
                        case 'F':{ KeyType = Key_F; }break;
                        case 'G':{ KeyType = Key_G; }break;
                        case 'H':{ KeyType = Key_H; }break;
                        case 'I':{ KeyType = Key_I; }break;
                        case 'J':{ KeyType = Key_J; }break;
                        case 'K':{ KeyType = Key_K; }break;
                        case 'L':{ KeyType = Key_L; }break;
                        case 'M':{ KeyType = Key_M; }break;
                        case 'N':{ KeyType = Key_N; }break;
                        case 'O':{ KeyType = Key_O; }break;
                        case 'P':{ KeyType = Key_P; }break;
                        case 'Q':{ KeyType = Key_Q; }break;
                        case 'R':{ KeyType = Key_R; }break;
                        case 'S':{ KeyType = Key_S; }break;
                        case 'T':{ KeyType = Key_T; }break;
                        case 'U':{ KeyType = Key_U; }break;
                        case 'V':{ KeyType = Key_V; }break;
                        case 'W':{ KeyType = Key_W; }break;
                        case 'X':{ KeyType = Key_X; }break;
                        case 'Y':{ KeyType = Key_Y; }break;
                        case 'Z':{ KeyType = Key_Z; }break;
                        
                        case VK_NUMPAD0: { KeyType = Key_Num0; }break;
                        case VK_NUMPAD1: { KeyType = Key_Num1; }break;
                        case VK_NUMPAD2: { KeyType = Key_Num2; }break;
                        case VK_NUMPAD3: { KeyType = Key_Num3; }break;
                        case VK_NUMPAD4: { KeyType = Key_Num4; }break;
                        case VK_NUMPAD5: { KeyType = Key_Num5; }break;
                        case VK_NUMPAD6: { KeyType = Key_Num6; }break;
                        case VK_NUMPAD7: { KeyType = Key_Num7; }break;
                        case VK_NUMPAD8: { KeyType = Key_Num8; }break;
                        case VK_NUMPAD9: { KeyType = Key_Num9; }break;
                        case VK_MULTIPLY: { KeyType = Key_Multiply; }break;
                        case VK_ADD: { KeyType = Key_Add; }break;
                        case VK_DIVIDE: { KeyType = Key_Divide; }break;
                        case VK_SUBTRACT: { KeyType = Key_Subtract; }break;
                        case VK_SEPARATOR: { KeyType = Key_Separator; }break;
                        case VK_DECIMAL: { KeyType = Key_Decimal; }break;
                        case VK_F1: {  KeyType = Key_F1; }break;
                        case VK_F2: {  KeyType = Key_F2; }break;
                        case VK_F3: {  KeyType = Key_F3; }break;
                        case VK_F4: {  KeyType = Key_F4; }break;
                        case VK_F5: {  KeyType = Key_F5; }break;
                        case VK_F6: {  KeyType = Key_F6; }break;
                        case VK_F7: {  KeyType = Key_F7; }break;
                        case VK_F8: {  KeyType = Key_F8; }break;
                        case VK_F9: {  KeyType = Key_F9; }break;
                        case VK_F10: {  KeyType = Key_F10; }break;
                        case VK_F11: {  KeyType = Key_F11; }break;
                        case VK_F12: {  KeyType = Key_F12; }break;
                        case VK_VOLUME_MUTE: { KeyType = Key_VolumeMute; }break;
                        case VK_VOLUME_UP: { KeyType = Key_VolumeUp; }break;
                        case VK_VOLUME_DOWN: { KeyType = Key_VolumeDown; }break;
                        
                        Win32ProcessKey(&Input->KeyStates[KeyType], IsDown);
                    }
                    
                    if(IsDown){
                        
                        if(AltKeyWasDown && VKey == VK_F4){
                            GlobalRunning = 0;
                        }
                        
                        if(AltKeyWasDown && VKey == VK_RETURN){
                            Win32ToggleFullscreen(&Win32);
                        }
                    }
                }
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
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }break;
        } //END SWITCH
    } //END_WHILE
}

INTERNAL_FUNCTION void
Win32ProcessInput(input* Input)
{
    POINT Point;
    BOOL GetCursorPosRes = GetCursorPos(&Point);
    BOOL ScreenToClientRes = ScreenToClient(Win32.Window, &Point);
    
    v2 MouseP = V2(Point.x, Point.y);
    Input->LastMouseP = Input->MouseP;
    Input->MouseP = MouseP;
    
    //NOTE(Dima): Processing mouse buttons
    DWORD Win32MouseKeyID[] = {
        VK_LBUTTON,
        VK_MBUTTON,
        VK_RBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };
    
    for(u32 MouseKeyIndex = 0;
        MouseKeyIndex < ARRAY_COUNT(Win32MouseKeyID);
        MouseKeyIndex++)
    {
        Input->KeyStates[MouseKey_Left + MouseKeyIndex].TransitionHappened = 0;
        SHORT WinMouseKeyState = GetKeyState(Win32MouseKeyID[MouseKeyIndex]);
        
        Win32ProcessKey(&Input->KeyStates[MouseKey_Left + MouseKeyIndex], WinMouseKeyState & (1 << 15));
    }
}

INTERNAL_FUNCTION void
Win32DisplayBitmapInWindow(
win32_state* Win,
HDC WindowDС,
int WindowWidth,
int WindowHeight)
{
    DWORD Style = GetWindowLong(Win->Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        StretchDIBits(
            WindowDС,
            0, 0, WindowWidth, WindowHeight,
            0, 0, Win->Bitmap.Width, Win->Bitmap.Height,
            Win->Bitmap.Pixels, &Win->BMI,
            DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        GetMonitorInfo(MonitorFromWindow(Win->Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo);;
        StretchDIBits(
            WindowDС,
            MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
            MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
            MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
            0, 0, Win->Bitmap.Width, Win->Bitmap.Height,
            Win->Bitmap.Pixels, &Win->BMI,
            DIB_RGB_COLORS, SRCCOPY);
    }
}

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

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    QueryPerformanceFrequency(&Win32.PerformanceFreqLI);
    Win32.OneOverPerformanceFreq = 1.0f / (float)Win32.PerformanceFreqLI.QuadPart;
    
    u32 MemoryBlockSize = Gibibytes(1);
    void* MemoryBlock = VirtualAlloc(
        0, 
        MemoryBlockSize,
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE);
    memory_region GlobalMem = InitMemoryRegion(MemoryBlock, MemoryBlockSize);
    
    WNDCLASSEXA WindowClass = {};
    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.lpszClassName = "MainWindowClassName";
    WindowClass.hInstance = hInstance;
    WindowClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowProcessing;
    
    RegisterClassExA(&WindowClass);
    
    int WindowWidth = 1366;
    WindowWidth = (WindowWidth + 3) & (~3);
    int WindowHeight = 768;
    
    int WindowCreateW;
    int WindowCreateH;
    
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
    
    Win32.Window = CreateWindowA(
        WindowClass.lpszClassName,
        "Joy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowCreateW,
        WindowCreateH,
        0, 0, 
        hInstance, 
        0);
    
    Win32.WindowWidth = WindowWidth;
    Win32.WindowHeight = WindowHeight;
    
    void* Win32BitmapMemory = PushSomeMem(&GlobalMem, WindowWidth * WindowHeight * 4, 64);
    Win32.Bitmap = AssetAllocateBitmapInternal(WindowWidth, WindowHeight, Win32BitmapMemory);
    Win32.BMI = {};
    BITMAPINFO* BMI = &Win32.BMI;
    BITMAPINFOHEADER* BMIHeader = &BMI->bmiHeader;
    BMIHeader->biSize=  sizeof(BITMAPINFOHEADER);
    BMIHeader->biWidth = WindowWidth;
    BMIHeader->biHeight = -WindowHeight;
    BMIHeader->biPlanes = 1;
    BMIHeader->biBitCount = 32;
    BMIHeader->biCompression = BI_RGB;
    BMIHeader->biSizeImage = 0;
    
    HDC GlDC = GetDC(Win32.Window);
    Win32.RenderContext = Win32InitOpenGL(GlDC);
    
    // NOTE(Dima): Initializing platform API
    InitDefaultPlatformAPI(&PlatformAPI);
    
    PlatformAPI.ReadFile = Win32ReadFile;
    PlatformAPI.WriteFile = Win32WriteFile;
    PlatformAPI.FreeFileMemory = Win32FreeFileMemory;
    PlatformAPI.ShowError = Win32ShowError;
    PlatformAPI.OutputString = Win32DebugOutputString;
    
    // NOTE(Dima): Initializing engine systems
    InitAssets(&GlobalAssets);
    render_stack RenderStack_ = InitRenderStack(&GlobalMem, Megabytes(1));
    render_stack* RenderStack = &RenderStack_;
    InitGui(
        &GlobalGui, 
        &GlobalInput, 
        &GlobalAssets, 
        &GlobalMem, 
        RenderStack, 
        WindowWidth, 
        WindowHeight);
    
    float Time = 0.0f;
    float DeltaTime = 0.016f;
    
    gui_layout* Lay = GetFirstLayout(&GlobalGui);
    
    //ShellExecuteA(NULL, "open", "http://www.microsoft.com", NULL, NULL, SW_SHOWNORMAL);
    
    GlobalRunning = 1;
    while(GlobalRunning){
        LARGE_INTEGER BeginClockLI;
        QueryPerformanceCounter(&BeginClockLI);
        
        Win32ProcessMessages(&GlobalInput);
        Win32ProcessInput(&GlobalInput);
        
        rc2 ClipRect = RcMinMax(V2(0.0f, 0.0f), V2(WindowWidth, WindowHeight));
        
        RenderStackBeginFrame(RenderStack);
        
        PushClearColor(RenderStack, V3(1.0f, 0.5f, 0.0f));
        
        PushBitmap(RenderStack, &GlobalAssets.Sunset, V2(100.0f, Sin(Time * 1.0f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetOrange, V2(200.0f, Sin(Time * 1.1f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetField, V2(300.0f, Sin(Time * 1.2f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.SunsetMountains, V2(400.0f, Sin(Time * 1.3f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.MountainsFuji, V2(500.0f, Sin(Time * 1.4f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.RoadClouds, V2(600.0f, Sin(Time * 1.5f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(RenderStack, &GlobalAssets.Sunrise, V2(700.0f, Sin(Time * 1.6f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        
        char FPSBuf[64];
        stbsp_sprintf(FPSBuf, "FPS %.2f, ms %.3f", 1.0f / DeltaTime, DeltaTime);
        
        static int LastFrameEntryCount = 0;
        static int LastFrameBytesUsed = 0;
        char StackInfo[256];
        stbsp_sprintf(StackInfo, "EntryCount: %d; BytesUsed: %d;", 
                      LastFrameEntryCount, 
                      LastFrameBytesUsed);
        
#if 1
        gui_state* Gui = &GlobalGui;
        
        GuiBeginPage(Gui, "Page1");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Page2");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Animation");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "Platform");
        GuiEndPage(Gui);
        
        GuiBeginPage(Gui, "GUI");
        GuiEndPage(Gui);
        
        //GuiUpdateWindows(Gui);
        
        GuiBeginLayout(Gui, Lay);
        GuiText(Gui, "Hello world");
        GuiText(Gui, FPSBuf);
        GuiText(Gui, StackInfo);
        GuiText(Gui, "I love Kate");
        GuiText(Gui, "I wish joy and happiness for everyone");
        
        LOCAL_AS_GLOBAL int RectCount = 0;
        GuiBeginRow(Gui);
        if(GuiButton(Gui, "Add")){
            RectCount++;
        }
        if(GuiButton(Gui, "Clear")){
            RectCount--;
            if(RectCount < 0){
                RectCount = 0;
            }
        }
        GuiEndRow(Gui);
        for(int i = 0; i < RectCount; i++){
            PushRect(RenderStack, RcMinDim(V2(100 + i * 50, 100), V2(40, 40)));
        }
        
        static b32 BoolButtonValue;
        GuiBeginRow(Gui);
        GuiBoolButton(Gui, "BoolButton", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButton123", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButton1234", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButtonasdfga", &BoolButtonValue);
        GuiBoolButton(Gui, "BoolButtonzxcvzxcb", &BoolButtonValue);
        GuiEndRow(Gui);
        
        static b32 BoolButtonOnOffValue;
        GuiBeginRow(Gui);
        GuiBeginColumn(Gui);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff1", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff2", &BoolButtonValue);
        GuiBoolButtonOnOff(Gui, "BoolButtonOnOff3", &BoolButtonValue);
        GuiEndColumn(Gui);
        
        GuiBeginColumn(Gui);
        static b32 CheckboxValue1;
        static b32 CheckboxValue2;
        static b32 CheckboxValue3;
        static b32 CheckboxValue4;
        GuiCheckbox(Gui, "Checkbox", &CheckboxValue1);
        GuiCheckbox(Gui, "Checkbox1", &CheckboxValue2);
        GuiCheckbox(Gui, "Checkbox2", &CheckboxValue3);
        GuiCheckbox(Gui, "Checkbox3", &CheckboxValue4);
        GuiEndColumn(Gui);
        GuiEndRow(Gui);
        
        GuiTooltip(Gui, "Hello world!", GlobalInput.MouseP);
        
        GuiPreRender(Gui);
        
        GuiEndLayout(&GlobalGui, Lay);
#endif
        
        LastFrameBytesUsed = RenderStack->Data.Used;
        LastFrameEntryCount = RenderStack->EntryCount;
        
        RenderMultithreaded(&PlatformAPI.HighPriorityQueue, RenderStack, &Win32.Bitmap);
        RenderMultithreadedRGBA2BGRA(&PlatformAPI.HighPriorityQueue, &Win32.Bitmap);
        
        Win32DisplayBitmapInWindow(&Win32, GlDC, WindowWidth, WindowHeight);
        
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        LARGE_INTEGER EndClockLI;
        QueryPerformanceCounter(&EndClockLI);
        u64 ClocksElapsed = EndClockLI.QuadPart - BeginClockLI.QuadPart;
        DeltaTime = (float)ClocksElapsed * Win32.OneOverPerformanceFreq;
        Time += DeltaTime;
        
        //SwapBuffers(GlDC);
    }
    
    //NOTE(dima): Cleanup
#if USE_VULKAN
    Win32CleanupVulkan(&Win32.Vulkan);
#endif
    
    FreePlatformAPI(&PlatformAPI);
    Win32FreeOpenGL(Win32.RenderContext);
    ReleaseDC(Win32.Window, GlDC);
    DestroyWindow(Win32.Window);
    VirtualFree(MemoryBlock, 0, MEM_RELEASE);
    
    return (0);
}
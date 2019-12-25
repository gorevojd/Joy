#include "win32_joy.h"
#include "joy_dirx.h"
#include <dsound.h>

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

GLOBAL_VARIABLE Win_State win32;
GLOBAL_VARIABLE b32 gRunning;
GLOBAL_VARIABLE Input gInput;
GLOBAL_VARIABLE Assets gAssets;
GLOBAL_VARIABLE Gui_State gGui;
GLOBAL_VARIABLE Gl_State gGL;
GLOBAL_VARIABLE DirX_State gDirX;
GLOBAL_VARIABLE DSound_State gDSound;

Platform platform;

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
    OutputDebugString(text);
}

PLATFORM_MEMALLOC(Win32MemAlloc){
    
    // NOTE(Dima): I'll allocate 1 extra page to hold info
    // NOTE(Dima): about allocation & win32_memory_block structure
    const mi pageSize = 4096;
    const mi pageSizeMask = 4095;
    mi toAllocSize = size + pageSize;
    
    // NOTE(Dima): Always put guards at the beginning and at the end
    // NOTE(Dima): For begin&end guard pages
    toAllocSize += 2 * pageSize;
    
    void* allocatedMemory = VirtualAlloc(0, toAllocSize, 
                                         MEM_COMMIT | MEM_RESERVE, 
                                         PAGE_READWRITE);
    Assert(allocatedMemory);
    
    Win_Memory_Region* region = (Win_Memory_Region*)allocatedMemory;
    
    
    void* beginGuardPage = (u8*)region + pageSize;
    DWORD oldProtectBegin;
    VirtualProtect(beginGuardPage, pageSize, PAGE_NOACCESS, &oldProtectBegin);
    
    void* result = (u8*)beginGuardPage + pageSize;
    
    void* endGuardPage = (u8*)result + ((size + pageSizeMask) & (~(pageSizeMask)));
    DWORD oldProtectEnd;
    VirtualProtect(endGuardPage, pageSize, PAGE_NOACCESS, &oldProtectEnd);
    
    // NOTE(Dima): Inserting region to list
    region->prev = &win32.memorySentinel;
    BeginTicketMutex(&win32.memoryMutex);
    region->next = win32.memorySentinel.next;
    
    region->prev->next = region;
    region->next->prev = region;
    EndTicketMutex(&win32.memoryMutex);
    
    // NOTE(Dima): Initializing region
    region->totalCommittedSize = (toAllocSize + pageSizeMask) & (~pageSizeMask);
    region->size = size;
    region->baseAddress = result;
    region->baseAddressOfAllocationBlock = allocatedMemory;
    
    return(result);
}

PLATFORM_MEMFREE(Win32MemFree){
    if(toFree){
        const mi pageSize = 4096;
        void* actualToFree = (void*)((u8*)toFree - 2 * pageSize);
        
        Win_Memory_Region* region = (Win_Memory_Region*)actualToFree;
        
        // NOTE(Dima): Removing from list
        BeginTicketMutex(&win32.memoryMutex);
        region->next->prev = region->prev;
        region->prev->next = region->next;
        EndTicketMutex(&win32.memoryMutex);
        
        VirtualFree(toFree, 0, MEM_RELEASE);
        
    }
}

INTERNAL_FUNCTION void
Win32ToggleFullscreen(Win_State* win32)
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

inline void Win32ProcessKey(KeyState* key, b32 isDown){
    if(key->endedDown != isDown){
        key->endedDown = isDown;
        
        key->transitionHappened = 1;
    }
}

INTERNAL_FUNCTION void 
Win32ProcessMessages(Input* input){
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
                
                //NOTE(dima): If state of key was changed
                if(wasDown != isDown){
                    u32 keyType;
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
                        
                        Win32ProcessKey(&input->keyStates[keyType], isDown);
                    }
                    
                    if(isDown){
                        
                        if(altKeyWasDown && vKey == VK_F4){
                            gRunning = 0;
                        }
                        
                        if(altKeyWasDown && vKey == VK_RETURN){
                            Win32ToggleFullscreen(&win32);
                        }
                    }
                }
            }break;
            
            case WM_LBUTTONDOWN:{
                
            }break;
            
            case WM_QUIT:{
                gRunning = 0;
            }break;
            
            case WM_CLOSE:{
                PostQuitMessage(0);
                gRunning = 0;
            }break;
            
            case WM_DESTROY:{
                PostQuitMessage(0);
                gRunning = 0;
            }break;
            
            default:{
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }break;
        } //END SWITCH
    } //END_WHILE
}

INTERNAL_FUNCTION void
Win32ProcessInput(Input* input)
{
    POINT point;
    BOOL getCursorPosRes = GetCursorPos(&point);
    BOOL screenToClientRec = ScreenToClient(win32.window, &point);
    
    v2 mouseP = V2(point.x, point.y);
    input->lastMouseP = input->mouseP;
    input->mouseP = mouseP;
    
    //NOTE(Dima): Processing mouse buttons
    DWORD win32MouseKeyID[] = {
        VK_LBUTTON,
        VK_MBUTTON,
        VK_RBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };
    
    for(u32 mouseKeyIndex = 0;
        mouseKeyIndex < ARRAY_COUNT(win32MouseKeyID);
        mouseKeyIndex++)
    {
        input->keyStates[MouseKey_Left + mouseKeyIndex].transitionHappened = 0;
        SHORT winMouseKeyState = GetKeyState(win32MouseKeyID[mouseKeyIndex]);
        
        Win32ProcessKey(&input->keyStates[MouseKey_Left + mouseKeyIndex], winMouseKeyState & (1 << 15));
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
            gRunning = 0;
        }break;
        
        case WM_QUIT:{
            gRunning = 0;
        }break;
        
        case WM_CLOSE:{
            gRunning = 0;
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
    // NOTE(Dima): Initializing platform API
    InitDefaultPlatformAPI(&platform);
    
    platform.ReadFile = Win32ReadFile;
    platform.WriteFile = Win32WriteFile;
    platform.FreeFileMemory = Win32FreeFileMemory;
    platform.ShowError = Win32ShowError;
    platform.OutputString = Win32DebugOutputString;
    platform.MemAlloc = Win32MemAlloc;
    platform.MemFree = Win32MemFree;
    
    QueryPerformanceFrequency(&win32.performanceFreqLI);
    win32.oneOverPerformanceFreq = 1.0f / (float)win32.performanceFreqLI.QuadPart;
    
    // NOTE(Dima): Initializing memory sentinel
    win32.memorySentinel = {};
    win32.memorySentinel.prev = &win32.memorySentinel;
    win32.memorySentinel.next = &win32.memorySentinel;
    
    Memory_Region gMem = {};
    
    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.lpszClassName = "MainWindowClassName";
    wndClass.hInstance = hInstance;
    wndClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = Win32WindowProcessing;
    
    RegisterClassExA(&wndClass);
    
    int windowWidth = 1366;
    windowWidth = (windowWidth + 3) & (~3);
    int windowHeight = 768;
    
    int windowCreateW;
    int windowCreateH;
    
    RECT ClientRect;
    ClientRect.left = 0;
    ClientRect.top = 0;
    ClientRect.right = windowWidth;
    ClientRect.bottom = windowHeight;
    BOOL WindowRectAdjusted = AdjustWindowRect(
        &ClientRect, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & (~WS_OVERLAPPED), 0);
    
    if(WindowRectAdjusted){
        windowCreateW = ClientRect.right - ClientRect.left;
        windowCreateH = ClientRect.bottom - ClientRect.top;
    }
    else{
        windowCreateW = windowWidth;
        windowCreateH = windowHeight;
    }
    
    win32.window = CreateWindowA(
        wndClass.lpszClassName,
        "Joy",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowCreateW,
        windowCreateH,
        0, 0, 
        hInstance, 
        0);
    
    win32.windowWidth = windowWidth;
    win32.windowHeight = windowHeight;
    
    void* winBmpMemory = PushSomeMem(&gMem, windowWidth * windowHeight * 4, 64);
    win32.bitmap = AssetAllocateBitmapInternal(windowWidth, windowHeight, winBmpMemory);
    win32.bmi = {};
    BITMAPINFO* bmi = &win32.bmi;
    BITMAPINFOHEADER* bmiHeader = &bmi->bmiHeader;
    bmiHeader->biSize=  sizeof(BITMAPINFOHEADER);
    bmiHeader->biWidth = windowWidth;
    bmiHeader->biHeight = -windowHeight;
    bmiHeader->biPlanes = 1;
    bmiHeader->biBitCount = 32;
    bmiHeader->biCompression = BI_RGB;
    bmiHeader->biSizeImage = 0;
    
    HDC glDC = GetDC(win32.window);
    win32.renderCtx = Win32InitOpenGL(glDC);
    
    // NOTE(Dima): Initializing engine systems
    GlInit(&gGL);
    InitAssets(&gAssets);
    DirXInit(&gDirX, 
             win32.window, 
             win32.windowWidth,
             win32.windowHeight);
    DSoundInit(&gDSound, win32.window);
    
    mi renderMemSize = Megabytes(2);
    void* renderMem = PushSomeMem(&gMem, renderMemSize);
    Render_Stack renderStack_ = InitRenderStack(renderMem, renderMemSize);
    Render_Stack* renderStack = &renderStack_;
    InitGui(
        &gGui, 
        &gInput, 
        &gAssets, 
        &gMem, 
        renderStack, 
        windowWidth, 
        windowHeight);
    
    float time = 0.0f;
    float deltaTime = 0.016f;
    
    Gui_Layout* lay = GetFirstLayout(&gGui);
    
    //ShellExecuteA(NULL, "open", "http://www.microsoft.com", NULL, NULL, SW_SHOWNORMAL);
    
    gRunning = 1;
    while(gRunning){
        LARGE_INTEGER beginClockLI;
        QueryPerformanceCounter(&beginClockLI);
        
        Win32ProcessMessages(&gInput);
        Win32ProcessInput(&gInput);
        
        rc2 ClipRect = RcMinMax(V2(0.0f, 0.0f), V2(windowWidth, windowHeight));
        
        RenderStackBeginFrame(renderStack);
        
        PushClearColor(renderStack, V3(1.0f, 0.5f, 0.0f));
        
#if 0        
        PushGradient(renderStack, RcMinDim(V2(10, 10), V2(900, 300)), 
                     ColorFromHex("#FF00FF"), ColorFromHex("#4b0082"),
                     RenderEntryGradient_Horizontal);
        PushGradient(renderStack, RcMinDim(V2(100, 400), V2(900, 300)), 
                     ColorFromHex("#FF00FF"), ColorFromHex("#4b0082"), 
                     RenderEntryGradient_Vertical);
#endif
        
        PushBitmap(renderStack, &gAssets.sunset, V2(100.0f, Sin(time * 1.0f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.sunsetOrange, V2(200.0f, Sin(time * 1.1f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.sunsetField, V2(300.0f, Sin(time * 1.2f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.sunsetMountains, V2(400.0f, Sin(time * 1.3f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.mountainsFuji, V2(500.0f, Sin(time * 1.4f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.roadClouds, V2(600.0f, Sin(time * 1.5f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        PushBitmap(renderStack, &gAssets.sunrise, V2(700.0f, Sin(time * 1.6f) * 250.0f + 320.0f), 300.0f, V4(1.0f, 1.0f, 1.0f, 1.0f));
        
        char FPSBuf[64];
        stbsp_sprintf(FPSBuf, "FPS %.2f, ms %.3f", 1.0f / deltaTime, deltaTime);
        
        static int lastFrameEntryCount = 0;
        static int lastFrameBytesUsed = 0;
        char StackInfo[256];
        stbsp_sprintf(StackInfo, "EntryCount: %d; BytesUsed: %d;", 
                      lastFrameEntryCount, 
                      lastFrameBytesUsed);
        
#if 1
        Gui_State* gui = &gGui;
        
        GuiBeginPage(gui, "Page1");
        GuiEndPage(gui);
        
        GuiBeginPage(gui, "Page2");
        GuiEndPage(gui);
        
        GuiBeginPage(gui, "Animation");
        GuiEndPage(gui);
        
        GuiBeginPage(gui, "Platform");
        GuiEndPage(gui);
        
        GuiBeginPage(gui, "GUI");
        GuiEndPage(gui);
        
        //GuiUpdateWindows(gui);
        
        GuiBeginLayout(gui, lay);
        GuiText(gui, "Hello world");
        GuiText(gui, FPSBuf);
        GuiText(gui, StackInfo);
        GuiText(gui, "I love Kate");
        GuiText(gui, "I wish joy and happiness for everyone");
        
        LOCAL_AS_GLOBAL int RectCount = 0;
        GuiBeginRow(gui);
        if(GuiButton(gui, "Add")){
            RectCount++;
        }
        if(GuiButton(gui, "Clear")){
            RectCount--;
            if(RectCount < 0){
                RectCount = 0;
            }
        }
        GuiEndRow(gui);
        for(int i = 0; i < RectCount; i++){
            PushRect(renderStack, RcMinDim(V2(100 + i * 50, 100), V2(40, 40)));
        }
        
        static b32 boolButtonValue;
        GuiBeginRow(gui);
        GuiBoolButton(gui, "boolButton", &boolButtonValue);
        GuiBoolButton(gui, "boolButton123", &boolButtonValue);
        GuiBoolButton(gui, "boolButton1234", &boolButtonValue);
        GuiBoolButton(gui, "boolButtonasdfga", &boolButtonValue);
        GuiBoolButton(gui, "boolButtonzxcvzxcb", &boolButtonValue);
        GuiEndRow(gui);
        
        static b32 boolButtonOnOffValue;
        GuiBeginRow(gui);
        GuiBeginColumn(gui);
        GuiBoolButtonOnOff(gui, "boolButtonOnOff", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "boolButtonOnOff1", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "boolButtonOnOff2", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "boolButtonOnOff3", &boolButtonValue);
        GuiEndColumn(gui);
        
        GuiBeginColumn(gui);
        static b32 checkboxValue1;
        static b32 checkboxValue2;
        static b32 checkboxValue3;
        static b32 checkboxValue4;
        GuiCheckbox(gui, "Checkbox", &checkboxValue1);
        GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
        GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
        GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
        GuiEndColumn(gui);
        
        GuiBeginColumn(gui);
        GuiCheckbox(gui, "Checkbox", &checkboxValue1);
        GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
        GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
        GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
        GuiEndColumn(gui);
        GuiEndRow(gui);
        
        GuiBoolButtonOnOff(gui, "BoolButtonOnOff4", &boolButtonValue);
        GuiCheckbox(gui, "Checkbox4", &checkboxValue4);
        
        GuiBeginRow(gui);
        GuiBeginColumn(gui);
        GuiBoolButtonOnOff(gui, "BoolButtonOnOff", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "BoolButtonOnOff1", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "BoolButtonOnOff2", &boolButtonValue);
        GuiBoolButtonOnOff(gui, "BoolButtonOnOff3", &boolButtonValue);
        GuiEndColumn(gui);
        
        GuiBeginColumn(gui);
        GuiCheckbox(gui, "Checkbox", &checkboxValue1);
        GuiCheckbox(gui, "Checkbox1", &checkboxValue2);
        GuiCheckbox(gui, "Checkbox2", &checkboxValue3);
        GuiCheckbox(gui, "Checkbox3", &checkboxValue4);
        GuiEndColumn(gui);
        
        
        GuiEndRow(gui);
        
        
        GuiTooltip(gui, "Hello world!", gInput.mouseP);
        
        GuiPreRender(gui);
        
        GuiEndLayout(gui, lay);
#endif
        
        lastFrameBytesUsed = renderStack->memUsed;
        lastFrameEntryCount = renderStack->entryCount;
        
        RenderMultithreaded(&platform.highPriorityQueue, renderStack, &win32.bitmap);
        
        GlOutputRender(&gGL, &win32.bitmap);
        
        LARGE_INTEGER endClockLI;
        QueryPerformanceCounter(&endClockLI);
        u64 clocksElapsed = endClockLI.QuadPart - beginClockLI.QuadPart;
        deltaTime = (float)clocksElapsed * win32.oneOverPerformanceFreq;
        time += deltaTime;
        
        //gDirX.devCtx->ClearRenderTargetView(gDirX.backBuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));
        
        //gDirX.swapChain->Present(0, 0);
        
        SwapBuffers(glDC);
    }
    
    //NOTE(dima): Cleanup
    GlFree(&gGL);
    
    DSoundFree(&gDSound);
    DirXFree(&gDirX);
    FreePlatformAPI(&platform);
    Win32FreeOpenGL(win32.renderCtx);
    ReleaseDC(win32.window, glDC);
    DestroyWindow(win32.window);
    
    return (0);
}
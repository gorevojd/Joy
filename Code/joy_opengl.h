#ifndef JOY_OPENGL_H
#define JOY_OPENGL_H

#include "joy_platform.h"

#include <Windows.h>
#include "joy_opengl_defs.h"
#include "joy_render.h"

#define GLGETU(uniftext) GL_GetUniform(GL->Programs[Result.ProgramIndex], uniftext)
#define GLGETP(index) GL->Programs[index]
#define GLGETPID(shader) GL->Programs[(shader).ProgramIndex].ID

struct gl_program{
    GLuint ID;
};

inline GLint GL_GetUniform(gl_program Program, char* Text){
    GLint Result = glGetUniformLocation(Program.ID, Text);
    
    return(Result);
}


struct gl_screen_shader{
    int ProgramIndex;
    
    GLint ScreenTextureLoc;
};

struct gl_state{
    gl_program Programs[256];
    int ProgramsCount;
    
    gl_screen_shader ScreenShader;
    
    GLuint ScreenVAO;
    GLuint ScreenVBO;
};

void GL_Init(gl_state* GL);
void GL_Free(gl_state* GL);
void GL_OutputRender(gl_state* GL, bmp_info* BlitBMP);

#endif
#ifndef JOY_OPENGL_H
#define JOY_OPENGL_H

#include "joy_platform.h"

#include <Windows.h>
#include "joy_opengl_defs.h"
#include "joy_render.h"

#define GLGETU(uniftext) Gl_GetUniform(gl->programs[result.programIndex], uniftext)
#define GLGETP(index) gl->programs[index]
#define GLGETPID(shader) gl->programs[(shader).programIndex].id

struct Gl_Program{
    GLuint id;
};

inline GLint Gl_GetUniform(Gl_Program program, char* text){
    GLint result = glGetUniformLocation(program.id, text);
    
    return(result);
}


struct Gl_Screen_Shader{
    int programIndex;
    
    GLint screenTextureLoc;
};

struct Gl_State{
    Gl_Program programs[256];
    int programsCount;
    
    Gl_Screen_Shader screenShader;
    
    GLuint screenVAO;
    GLuint screenVBO;
};

void GlInit(Gl_State* gl);
void GlFree(Gl_State* gl);
void GlOutputRender(Gl_State* GL, Bmp_Info* blitBMP);

#endif
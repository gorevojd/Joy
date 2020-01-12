#ifndef JOY_OPENGL_H
#define JOY_OPENGL_H

#include "joy_platform.h"

#include <Windows.h>
#include "joy_opengl_defs.h"
#include "joy_render.h"


#define GLGETU(uniftext) GlGetUniform(gl->Programs[Result.ProgramIndex], uniftext)
#define GLGET_HELP(a) #a
#define GLGETUNAME(unamewithoutloc) Result.##unamewithoutloc##Loc = GlGetUniform(gl->Programs[Result.ProgramIndex], GLGET_HELP(unamewithoutloc))
#define GLGETA(attrtext) GlGetAttribLoc(gl->Programs[Result.ProgramIndex], attrtext)
#define GLGETANAME(atxtwithouattrloc) Result.##atxtwithouattrloc##AttrLoc = GlGetAttribLoc(gl->Programs[Result.ProgramIndex], GLGET_HELP(atxtwithouattrloc))
#define GLGETP(index) gl->Programs[index]
#define GLGETPID(shader) gl->Programs[(shader).ProgramIndex].ID

inline b32 GlArrayIsValid(GLint Arr){
    b32 Result = 1;
    
    if(Arr == -1){
        Result = 0;
    }
    
    return(Result);
}

struct Gl_Program{
    GLuint ID;
};

inline GLint GlGetUniform(Gl_Program program, char* text){
    GLint Result = glGetUniformLocation(program.ID, text);
    
    return(Result);
}

inline GLint GlGetAttribLoc(Gl_Program program, char* text){
    GLint Result = glGetAttribLocation(program.ID, text);
    
    return(Result);
}


struct Gl_Screen_Shader{
    int ProgramIndex;
    
    GLint ScreenTextureLoc;
};

struct Gl_Simple_Shader{
    int ProgramIndex;
    
    GLint ModelLoc;
    GLint ViewLoc;
    GLint ProjectionLoc;
    
    GLint PAttrLoc;
    GLint UVAttrLoc;
    GLint NAttrLoc;
    GLint TAttrLoc;
    GLint CAttrLoc;
};

struct Gl_GuiRect_Shader{
    int ProgramIndex;
    
    GLint BitmapLoc;
    GLint BitmapIsSetLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct Gl_State{
    Gl_Program Programs[256];
    int ProgramsCount;
    
    Gl_Screen_Shader ScreenShader;
    Gl_Simple_Shader SimpleShader;
    Gl_GuiRect_Shader GuiRectShader;
    
    GLuint ScreenVAO;
    GLuint ScreenVBO;
    
    GLuint MeshVAO;
    GLuint MeshVBO;
    
    GLuint GuiRectVAO;
    GLuint GuiRectVBO;
};

void GlInit(Gl_State* gl);
void GlFree(Gl_State* gl);
void GlOutputRender(Gl_State* GL, Render_Stack* stack);
void GlOutputCommands(Gl_State* gl, Render_Stack* stack);

#endif
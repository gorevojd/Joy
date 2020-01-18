#ifndef JOY_OPENGL_H
#define JOY_OPENGL_H

#include "joy_platform.h"

#include <Windows.h>
#include "joy_opengl_defs.h"
#include "joy_render.h"
#include "joy_assets.h"


#define GLGET_HELP(a) #a
#define GLGETU(unamewithoutloc) Result.##unamewithoutloc##Loc = glGetUniformLocation(Result.ID, GLGET_HELP(unamewithoutloc))
#define GLGETA(atxtwithouattrloc) Result.##atxtwithouattrloc##AttrLoc = glGetAttribLocation(Result.ID, GLGET_HELP(atxtwithouattrloc))
//#define GLGETP(index) gl->Programs[index]
//#define GLGETPID(shader) gl->Programs[(shader).ProgramIndex].ID

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

enum Gl_Shader_Type{
    GlShader_Simple,
    GlShader_Compute,
};

struct Gl_Shader{
    GLuint ID;
    u32 Type;
    int _InternalProgramIndex;
    
    void Use(){
        glUseProgram(ID);
    }
    
    void SetV3(GLint Loc, float x, float y, float z){
        glUniform3f(Loc, x, y, z);
    }
    
    void SetV3(GLint Loc, v3 A){
        glUniform3f(Loc, A.x, A.y, A.z);
    }
    
    void SetV4(GLint Loc, float x, float y, float z, float w){
        glUniform4f(Loc, x, y, z, w);
    }
    
    void SetV4(GLint Loc, v4 A){
        glUniform4f(Loc, A.x, A.y, A.z, A.w);
    }
    
    void SetM44(GLint Loc, float* Data){
        glUniformMatrix4fv(Loc, 1, JOY_TRUE, Data);
    }
};

struct Gl_Screen_Shader : public Gl_Shader{
    GLint ScreenTextureLoc;
};

struct Gl_Simple_Shader : public Gl_Shader{
    GLint ModelLoc;
    GLint ViewLoc;
    GLint ProjectionLoc;
    
    GLint PAttrLoc;
    GLint UVAttrLoc;
    GLint NAttrLoc;
    GLint TAttrLoc;
    GLint CAttrLoc;
};

struct Gl_GuiRect_Shader : public Gl_Shader{
    GLint BitmapLoc;
    GLint BitmapIsSetLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct Gl_GuiGeom_Shader : public Gl_Shader{
    GLint BitmapLoc;
    GLint TriangleGeomTypesLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct Gl_State{
    Gl_Program Programs[256];
    int ProgramsCount;
    
    GLuint LargeAtlasHandle;
    
    Gl_Screen_Shader ScreenShader;
    Gl_Simple_Shader SimpleShader;
    Gl_GuiRect_Shader GuiRectShader;
    Gl_GuiGeom_Shader GuiGeomShader;
    
    GLuint ScreenVAO;
    GLuint ScreenVBO;
    
    GLuint MeshVAO;
    GLuint MeshVBO;
    
    GLuint GuiGeomVAO;
    GLuint GuiGeomVBO;
    GLuint GuiGeomEBO;
    GLuint GuiGeomTB;
    
    GLuint GuiRectVAO;
    GLuint GuiRectVBO;
    
    m44 GuiOrtho;
};

void GlInit(Gl_State* gl, Assets* assets);
void GlFree(Gl_State* gl);
void GlOutputRender(Gl_State* GL, Render_State* render);
void GlOutputCommands(Gl_State* gl, Render_Stack* stack);

#endif
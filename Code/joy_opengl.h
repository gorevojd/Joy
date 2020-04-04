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

struct gl_program{
    GLuint ID;
};

inline GLint GlGetUniform(gl_program program, char* text){
    GLint Result = glGetUniformLocation(program.ID, text);
    
    return(Result);
}

inline GLint GlGetAttribLoc(gl_program program, char* text){
    GLint Result = glGetAttribLocation(program.ID, text);
    
    return(Result);
}

enum gl_shader_type{
    GlShader_Simple,
    GlShader_Compute,
};

struct gl_shader{
    GLuint ID;
    u32 Type;
    int _InternalProgramIndex;
    
    void Use(){
        glUseProgram(ID);
    }
    
    void SetBool(GLint Loc, b32 Value){
        glUniform1i(Loc, Value);
    }
    
    void SetInt(GLint Loc, int Value){
        glUniform1i(Loc, Value);
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
        glUniformMatrix4fv(Loc, 1, true, Data);
    }
};

struct Gl_Screen_Shader : public gl_shader{
    GLint ScreenTextureLoc;
};

struct Gl_Simple_Shader : public gl_shader{
    GLint ModelLoc;
    GLint ViewLoc;
    GLint ProjectionLoc;
    GLint HasSkinningLoc;
    GLint BoneTransformsLoc;
    GLint PassedBonesCountLoc;
    
    GLint AlbedoLoc;
    GLint NormalsLoc;
    GLint NormalsIsSetLoc;
    GLint AlbedoIsSetLoc;
    
    GLint PAttrLoc;
    GLint UVAttrLoc;
    GLint NAttrLoc;
    GLint TAttrLoc;
    GLint WeightsAttrLoc;
    GLint BoneIDsAttrLoc;
};

struct Gl_GuiRect_Shader : public gl_shader{
    GLint BitmapLoc;
    GLint BitmapIsSetLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct Gl_GuiGeom_Shader : public gl_shader{
    GLint BitmapLoc;
    GLint TriangleGeomTypesLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct gl_state{
    gl_program Programs[256];
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

void GlInit(gl_state* gl, assets* Assets);
void GlFree(gl_state* gl);
void GlOutputRender(gl_state* GL, render_state* Render);

#endif
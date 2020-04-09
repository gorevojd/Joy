#ifndef JOY_OPENGL_H
#define JOY_OPENGL_H

#include "joy_platform.h"

#include <Windows.h>
#include "joy_opengl_defs.h"
#include "joy_render.h"
#include "joy_assets.h"


#define GLGET_HELP(a) #a
#define GLGETU(unamewithoutloc) Result.##unamewithoutloc##Loc = glGetUniformLocation(Result.Shader.ID, GLGET_HELP(unamewithoutloc))
#define GLGETA(atxtwithouattrloc) Result.##atxtwithouattrloc##AttrLoc = glGetAttribLocation(Result.Shader.ID, GLGET_HELP(atxtwithouattrloc))
//#define GLGETP(index) gl->Programs[index]
//#define GLGETPID(shader) gl->Programs[(shader).ProgramIndex].ID

inline b32 ArrayIsValid(GLint Arr){
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
};

struct gl_screen_shader{
    gl_shader Shader;
    
    GLint ScreenTextureLoc;
};

struct gl_simple_shader{
    gl_shader Shader;
    
    GLint ModelLoc;
    GLint ViewLoc;
    GLint ProjectionLoc;
    GLint HasSkinningLoc;
    GLint BoneTransformsLoc;
    GLint BonesCountLoc;
    
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

struct gl_guirect_shader{
    gl_shader Shader;
    
    GLint BitmapLoc;
    GLint BitmapIsSetLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct gl_guigeom_shader{
    gl_shader Shader;
    
    GLint BitmapLoc;
    GLint TriangleGeomTypesLoc;
    GLint ProjectionLoc;
    
    GLint PUVAttrLoc;
    GLint CAttrLoc;
};

struct gl_debuggeom_shader{
    gl_shader Shader;
    
    GLint ViewProjectionLoc;
    
    GLint PAttrLoc;
    GLint ColorAttrLoc;
};

struct gl_state{
    gl_program Programs[256];
    int ProgramsCount;
    
    GLuint LargeAtlasHandle;
    
    gl_screen_shader ScreenShader;
    gl_simple_shader SimpleShader;
    gl_guirect_shader GuiRectShader;
    gl_guigeom_shader GuiGeomShader;
    gl_debuggeom_shader DebugGeomShader;
    
    GLuint ScreenVAO;
    GLuint ScreenVBO;
    
    GLuint MeshVAO;
    GLuint MeshVBO;
    
    GLuint DEBUGGeomVAO;
    GLuint DEBUGGeomVBO;
    
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
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
    
    GLint UVInvertYLoc;
    GLint ScreenTextureLoc;
};

struct gl_resolve_shader{
    gl_shader Shader;
    
    GLint UVInvertYLoc;
    GLint TextureToResolveLoc;
    GLint TextureResolveTypeLoc;
    GLint FarNearLoc;
};

struct gl_simple_shader{
    gl_shader Shader;
    
    GLint ModelLoc;
    GLint ViewLoc;
    GLint ProjectionLoc;
    GLint HasSkinningLoc;
    GLint BoneTransformsLoc;
    GLint BonesCountLoc;
    
    GLint AlbedoColorLoc;
    GLint AlbedoLoc;
    GLint NormalsLoc;
    GLint SpecularLoc;
    GLint EmissiveLoc;
    GLint TexturesSetFlagsLoc;
    
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

struct gl_guigeom_lines_shader{
    gl_shader Shader;
    
    GLint ProjectionLoc;
    GLint ColorsTextureLoc;
    
    GLint PAttrLoc;
};

struct gl_lines_shader{
    gl_shader Shader;
    
    GLint ViewProjectionLoc;
    GLint ColorsTextureLoc;
    
    GLint PAttrLoc;
};

struct gl_lighting_shader{
    gl_shader Shader;
    
    GLint AspectRatioLoc;
    GLint UVInvertYLoc;
    GLint FarNearLoc;
    GLint FOVRadiansLoc;
    
    GLint GNormalMetalRoughLoc;
    GLint GAlbedoSpecLoc;
    GLint GDepthTexLoc;
    GLint SSAOInputLoc;
    
    GLint FogEnabledLoc;
    GLint FogColorLoc;
    GLint FogDensityLoc;
    GLint FogGradientLoc;
};

struct gl_ssao_shader
{
    gl_shader Shader;
    
    GLint WidthHeightLoc;
    GLint UVInvertYLoc;
    
    GLint FarNearLoc;
    GLint DepthTexLoc;
    GLint NormalMetalRoughTexLoc;
    GLint FOVRadiansLoc;
    GLint PerspProjCoefsLoc;
    
    GLint SSAOKernelSamplesCountLoc;
    GLint SSAOKernelRadiusLoc;
    GLint SSAOKernelBufLoc;
    GLint SSAONoiseTexLoc;
    GLint SSAOContributionLoc;
    GLint SSAORangeCheckLoc;
};

struct gl_filter_shader
{
    gl_shader Shader;
    
    GLint UVInvertYLoc;
    GLint InputTexLoc;
    GLint FilterLoc;
    GLint FilterTypeLoc;
};

struct gl_state{
    gl_program Programs[256];
    int ProgramsCount;
    
    GLuint LargeAtlasHandle;
    
    gl_screen_shader ScreenShader;
    gl_simple_shader SimpleShader;
    gl_guirect_shader GuiRectShader;
    gl_guigeom_shader GuiGeomShader;
    gl_guigeom_lines_shader GuiLinesShader;
    gl_lines_shader LinesShader;
    gl_resolve_shader ResolveShader;
    gl_lighting_shader LightingShader;
    gl_ssao_shader SSAOShader;
    gl_filter_shader FilterShader;
    
    GLuint ScreenVAO;
    GLuint ScreenVBO;
    
    GLuint MeshVAO;
    GLuint MeshVBO;
    
    GLuint GuiRectVAO;
    GLuint GuiRectVBO;
    
    // NOTE(Dima): Lines geometry
    GLuint LinesVAO;
    // NOTE(Dima): 0 - with depth, 1 - no-depth
    GLuint LinesVBO[2];
    GLuint LinesTBO[2];
    
    // NOTE(Dima): Gui geometry
    GLuint GuiGeomVAO;
    GLuint GuiGeomVBO;
    GLuint GuiGeomEBO;
    GLuint GuiGeomTB;
    
    // NOTE(Dima): Gui lines geometry
    GLuint GuiLinesVAO;
    GLuint GuiLinesVBO;
    GLuint GuiLinesColorsTBO;
    
    // NOTE(Dima): GBuffer
    GLuint GBuffer;
    GLuint GBufferDepthTex;
    GLuint GNormalMetalRoughTex;
    GLuint GAlbedoSpecTex;
    
    // NOTE(Dima): SSAO
    GLuint SSAO_FBO;
    GLuint SSAO_Tex;
    
    GLuint SSAOBlur_FBO;
    GLuint SSAOBlur_Tex;
    
    GLuint SSAONoiseTex;
    GLuint SSAOKernelTex;
    GLuint SSAOKernelBuf;
    
    m44 GuiOrtho;
};

#endif
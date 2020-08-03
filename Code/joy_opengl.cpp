#include "joy_opengl.h"

INTERNAL_FUNCTION GLuint GlLoadFromSourceCompute(char* ComputeCode){
    
    char InfoLog[1024];
	int Success;
    
	GLuint ComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(ComputeShader, 1, &ComputeCode, 0);
	glCompileShader(ComputeShader);
	
	glGetShaderiv(ComputeShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(ComputeShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        Platform.OutputString(InfoLog);
        Assert(Success);
    }
    
    
	GLuint Program = glCreateProgram();
	glAttachShader(Program, ComputeShader);
    glLinkProgram(Program);
    
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(Program, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        Platform.OutputString(InfoLog);
        Assert(Success);
	}
    
	glDeleteShader(ComputeShader);
    
	return(Program);
}

INTERNAL_FUNCTION gl_shader GlLoadProgram(gl_state* GL, char* VertexPath, char* FragmentPath, char* GeometryPath = 0) {
    Assert(GL->ProgramsCount < ARRAY_COUNT(GL->Programs));
	int resultIndex = GL->ProgramsCount;
    gl_program* result = GL->Programs + GL->ProgramsCount++;
    
    gl_shader ResultShader = {};
    
	Platform_Read_File_Result vFile = Platform.ReadFile(VertexPath);
	Platform_Read_File_Result fFile = Platform.ReadFile(FragmentPath);
    Platform_Read_File_Result gFile = {};
    char* toPassGeometryData = 0;
    if(GeometryPath){
        gFile = Platform.ReadFile(GeometryPath);
        toPassGeometryData = (char*)gFile.data;
    }
    
    char* VertexSource = (char*)vFile.data;
    char* FragmentSource = (char*)fFile.data;
    char* GeometrySource = toPassGeometryData;
    
    char InfoLog[1024];
	int Success;
	
	GLuint VertexShader;
	GLuint FragmentShader;
	GLuint GeometryShader;
    GLuint Program;
    
	VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(VertexShader, 1, &VertexSource, 0);
	glCompileShader(VertexShader);
	
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(VertexShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        fprintf(stderr, "Error while loading vertex shader(%s)\n%s\n", VertexPath, InfoLog);
        Platform.OutputString(InfoLog);
        Assert(Success);
    }
    
	FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentSource, 0);
	glCompileShader(FragmentShader);
    
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(FragmentShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        fprintf(stderr, "Error while loading fragment shader(%s)\n%s\n", FragmentPath, InfoLog);
        Platform.OutputString(InfoLog);
        Assert(Success);
    }
    
    if(GeometrySource){
        GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(GeometryShader, 1, &GeometrySource, 0);
        glCompileShader(GeometryShader);
        
        glGetShaderiv(GeometryShader, GL_COMPILE_STATUS, &Success);
        if (!Success) {
            glGetShaderInfoLog(GeometryShader, sizeof(InfoLog), 0, InfoLog);
            //TODO(dima): Logging
            fprintf(stderr, "Error while loading geometry shader(%s)\n%s\n", 
                    GeometryPath, InfoLog);
            Platform.OutputString(InfoLog);
            Assert(Success);
        }
    }
    
	Program = glCreateProgram();
	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);
	if(GeometrySource){
        glAttachShader(Program, GeometryShader);
    }
    glLinkProgram(Program);
    
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(Program, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        fprintf(stderr, "Error while linking shader program\n%s\n", InfoLog);
        Platform.OutputString(InfoLog);
        Assert(Success);
	}
    
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
    if(GeometrySource){
        glDeleteShader(GeometryShader);
    }
    
    ResultShader.ID = Program;
    ResultShader.Type = GlShader_Simple;
    ResultShader._InternalProgramIndex = resultIndex;
    
	Platform.FreeFileMemory(&vFile);
	Platform.FreeFileMemory(&fFile);
    if(GeometryPath){
        Platform.FreeFileMemory(&gFile);
    }
    
	return(ResultShader);
}

void UseShader(gl_shader* Shader){
    glUseProgram(Shader->ID);
}

void UseShaderEnd(){
    glUseProgram(0);
}

void UniformBool(GLint Loc, b32 Value){
    glUniform1i(Loc, Value);
}

void UniformInt(GLint Loc, int Value){
    glUniform1i(Loc, Value);
}


void UniformFloat(GLint Loc, float Value){
    glUniform1f(Loc, Value);
}

void UniformVec2(GLint Loc, float x, float y){
    glUniform2f(Loc, x, y);
}


void UniformVec2(GLint Loc, v2 Vector){
    glUniform2f(Loc, Vector.x, Vector.y);
}

void UniformVec3(GLint Loc, float x, float y, float z){
    glUniform3f(Loc, x, y, z);
}

void UniformVec3(GLint Loc, v3 A){
    glUniform3f(Loc, A.x, A.y, A.z);
}

void UniformVec4(GLint Loc, float x, float y, float z, float w){
    glUniform4f(Loc, x, y, z, w);
}

void UniformVec4(GLint Loc, v4 A){
    glUniform4f(Loc, A.x, A.y, A.z, A.w);
}

void UniformMatrix4x4(GLint Loc, float* Data)
{
    glUniformMatrix4fv(Loc, 1, true, Data);
}

void UniformMatrixArray4x4(GLint Loc, int Count, m44* Array)
{
    glUniformMatrix4fv(Loc, Count, true, (const GLfloat*)Array);
}

inline void UniformTextureInternal(GLint Loc, GLuint Texture, GLint Slot, GLint Target)
{
    glActiveTexture(GL_TEXTURE0 + Slot);
    glBindTexture(Target, Texture);
    glUniform1i(Loc, Slot);
}

void UniformTexture2D(GLint Loc, GLuint Texture, GLint Slot)
{
    UniformTextureInternal(Loc, Texture, Slot, GL_TEXTURE_2D);
}

void UniformTextureBuffer(GLint Loc, GLuint Texture, GLint Slot)
{
    UniformTextureInternal(Loc, Texture, Slot, GL_TEXTURE_BUFFER);
}

#define LOAD_SHADER_FUNC(name) void name(gl_state* GL, char* PathV, char* PathF)

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadScreenShader){
    gl_screen_shader& Result = GL->ScreenShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(ScreenTexture);
    GLGETU(UVInvertY);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadResolveShader){
    gl_resolve_shader& Result = GL->ResolveShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(TextureToResolve);
    GLGETU(TextureResolveType);
    GLGETU(UVInvertY);
    GLGETU(FarNear);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadLightingShader)
{
    gl_lighting_shader& Result = GL->LightingShader;
    
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(AspectRatio);
    GLGETU(UVInvertY);
    GLGETU(FOVRadians);
    
    GLGETU(GNormalMetalRough);
    GLGETU(GAlbedoSpec);
    GLGETU(GDepthTex);
    GLGETU(SSAOInput);
    
    GLGETU(FarNear);
    
    GLGETU(FogEnabled);
    GLGETU(FogColor);
    GLGETU(FogDensity);
    GLGETU(FogGradient);
}


INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadSSAOShader)
{
    gl_ssao_shader& Result = GL->SSAOShader;
    
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(WidthHeight);
    GLGETU(UVInvertY);
    GLGETU(FOVRadians);
    GLGETU(PerspProjCoefs);
    
    GLGETU(FarNear);
    GLGETU(DepthTex);
    GLGETU(NormalMetalRoughTex);
    
    GLGETU(SSAOKernelSamplesCount);
    GLGETU(SSAOKernelRadius);
    GLGETU(SSAOKernelBuf);
    GLGETU(SSAONoiseTex);
    GLGETU(SSAOContribution);
    GLGETU(SSAORangeCheck);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadFilterShader)
{
    gl_filter_shader& Result = GL->FilterShader;
    
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(UVInvertY);
    GLGETU(InputTex);
    GLGETU(Filter);
    GLGETU(FilterType);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadSimpleShader){
    gl_simple_shader& Result = GL->SimpleShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(Model);
    GLGETU(View);
    GLGETU(Projection);
    GLGETU(HasSkinning);
    GLGETU(BoneTransforms);
    GLGETU(BonesCount);
    
    GLGETU(AlbedoColor);
    GLGETU(Albedo);
    GLGETU(Normals);
    GLGETU(Emissive);
    GLGETU(Specular);
    GLGETU(TexturesSetFlags);
    
    GLGETA(P);
    GLGETA(UV);
    GLGETA(N);
    GLGETA(T);
    GLGETA(Weights);
    GLGETA(BoneIDs);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadGuiRectShader)
{
    gl_guirect_shader& Result = GL->GuiRectShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(Projection);
    GLGETU(BitmapIsSet);
    GLGETU(Bitmap);
    
    GLGETA(PUV);
    GLGETA(C);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadGuiGeomShader)
{
    gl_guigeom_shader& Result = GL->GuiGeomShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(Projection);
    GLGETU(Bitmap);
    GLGETU(TriangleGeomTypes);
    
    GLGETA(PUV);
    GLGETA(C);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadGuiLinesShader)
{
    gl_guigeom_lines_shader& Result = GL->GuiLinesShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(Projection);
    
    GLGETA(P);
}

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadLinesShader)
{
    gl_lines_shader& Result = GL->LinesShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(ViewProjection);
    GLGETU(ColorsTexture);
    
    GLGETA(P);
}

INTERNAL_FUNCTION void BindBufferAndFill(GLenum Target, GLenum Usage,
                                         void* Data, size_t DataSize,
                                         GLuint BufferName)
{
    glBindBuffer(Target, BufferName);
    
    GLint CurrentBufSize;
    glGetBufferParameteriv(Target, 
                           GL_BUFFER_SIZE,
                           &CurrentBufSize);
    
    if(DataSize > CurrentBufSize){
        // NOTE(Dima): Reallocating or initializing at the first time
        glBufferData(Target, DataSize + Kilobytes(5), 0, Usage);
    }
    
    glBufferSubData(Target, 0, DataSize, Data);
}

INTERNAL_FUNCTION GLuint GenerateAndBindBufferTexture(GLint GlTextureUnit, 
                                                      GLuint GlComponentType, 
                                                      GLuint TBO)
{
    GLuint TexBufTex;
    glGenTextures(1, &TexBufTex);
    
    glActiveTexture(GlTextureUnit);
    glBindTexture(GL_TEXTURE_BUFFER, TexBufTex);
    
    glTexBuffer(GL_TEXTURE_BUFFER, GlComponentType, TBO);
    
    return(TexBufTex);
}

INTERNAL_FUNCTION void InitVertexAttribFloat(GLint AttrLoc, 
                                             int ComponentCount,
                                             size_t Stride,
                                             size_t Offset)
{
    if(ArrayIsValid(AttrLoc)){
        glEnableVertexAttribArray(AttrLoc);
        glVertexAttribPointer(AttrLoc,
                              ComponentCount, 
                              GL_FLOAT, 
                              GL_FALSE,
                              Stride, 
                              (GLvoid*)(Offset));
    }
}

INTERNAL_FUNCTION void InitVertexAttribUint(GLint AttrLoc, 
                                            int ComponentCount,
                                            size_t Stride,
                                            size_t Offset)
{
    if(ArrayIsValid(AttrLoc)){
        glEnableVertexAttribArray(AttrLoc);
        glVertexAttribIPointer(AttrLoc,
                               ComponentCount, 
                               GL_UNSIGNED_INT, 
                               Stride, 
                               (GLvoid*)(Offset));
    }
}

INTERNAL_FUNCTION void FreeVertexAttrib(GLint AttrLoc){
    if(ArrayIsValid(AttrLoc)){
        glDisableVertexAttribArray(AttrLoc);
    }
}

INTERNAL_FUNCTION GLuint GlAllocateTexture(bmp_info* bmp){
    GLuint GenerateTex = 0;
    
    if(!bmp->Handle){
        
        glGenTextures(1, &GenerateTex);
        glBindTexture(GL_TEXTURE_2D, GenerateTex);
        
        glTexImage2D(
                     GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     bmp->Width,
                     bmp->Height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     bmp->Pixels);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        bmp->Handle = GenerateTex;
    }
    
    return(GenerateTex);
}

INTERNAL_FUNCTION void CheckFramebufferStatus()
{
    GLenum FramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    Assert(FramebufferStatus == GL_FRAMEBUFFER_COMPLETE);
    if(FramebufferStatus != GL_FRAMEBUFFER_COMPLETE){
        Platform.OutputString("Can not create render framebuffer\n");
    }
}

INTERNAL_FUNCTION inline void GlAddMeshHandle(mesh_handles* Handles, u32 HandleType, size_t Handle)
{
    ASSERT(Handles->Count < ARRAY_COUNT(Handles->Handles));
    
    int TargetIndex = Handles->Count++;
    
    Handles->Handles[TargetIndex] = Handle;
    Handles->HandlesTypes[TargetIndex] = HandleType;
}

INTERNAL_FUNCTION inline void GlFreeMeshHandles(mesh_handles* Handles){
    if(Handles->Allocated){
        for(int i = Handles->Count - 1;
            i >= 0;
            i--)
        {
            switch(Handles->HandlesTypes[i]){
                case MeshHandle_VertexArray:{
                    GLuint Arr = Handles->Handles[i];
                    glDeleteVertexArrays(1, &Arr);
                }break;
                
                case MeshHandle_Buffer:{
                    GLuint Buf = Handles->Handles[i];
                    glDeleteBuffers(1, &Buf);
                }break;
                
                default:{
                    INVALID_CODE_PATH;
                }break;
            }
            
            Handles->Handles[i] = 0;
        }
    }
    
    Handles->Allocated = false;
}

INTERNAL_FUNCTION mesh_handles* GlAllocateMesh(gl_state* GL, mesh_info* Mesh){
    mesh_handles* Result = &Mesh->Handles;
    
    if(!Mesh->Handles.Allocated){
        *Result = {};
        
        GLuint VAO, VBO, EBO;
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        size_t Stride = Mesh->TypeCtx.VertexTypeSize;
        
        glBindVertexArray(VAO);
        BindBufferAndFill(GL_ARRAY_BUFFER,
                          GL_STATIC_DRAW,
                          Mesh->Vertices,
                          Mesh->VerticesCount * Stride,
                          VBO);
        
        BindBufferAndFill(GL_ELEMENT_ARRAY_BUFFER,
                          GL_STATIC_DRAW,
                          Mesh->Indices,
                          Mesh->IndicesCount * sizeof(u32),
                          EBO);
        
#define GLGETOFFSET(index) (GLvoid*)((index) * sizeof(GLfloat))
        
        InitVertexAttribFloat(GL->SimpleShader.PAttrLoc,
                              3, Stride, 
                              Mesh->TypeCtx.OffsetP);
        
        InitVertexAttribFloat(GL->SimpleShader.UVAttrLoc,
                              2, Stride, 
                              Mesh->TypeCtx.OffsetUV);
        
        InitVertexAttribFloat(GL->SimpleShader.NAttrLoc,
                              3, Stride, 
                              Mesh->TypeCtx.OffsetN);
        
        InitVertexAttribFloat(GL->SimpleShader.TAttrLoc,
                              3, Stride, 
                              Mesh->TypeCtx.OffsetT);
        
        if(Mesh->TypeCtx.MeshType == Mesh_Skinned){
            InitVertexAttribFloat(GL->SimpleShader.WeightsAttrLoc,
                                  4, Stride,
                                  Mesh->TypeCtx.OffsetWeights);
            
            InitVertexAttribUint(GL->SimpleShader.BoneIDsAttrLoc,
                                 1, Stride,
                                 Mesh->TypeCtx.OffsetBoneIDs);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        GlAddMeshHandle(Result, MeshHandle_VertexArray, VAO);
        GlAddMeshHandle(Result, MeshHandle_Buffer, VBO);
        GlAddMeshHandle(Result, MeshHandle_Buffer, EBO);
        
        Result->Allocated = true;
    }
    
    return(Result);
}



INTERNAL_FUNCTION void GlInit(gl_state* GL, 
                              render_state* Render, 
                              asset_system* Assets)
{
    GL->ProgramsCount = 0;
    LoadScreenShader(GL, 
                     "../Data/Shaders/screen.vs",
                     "../Data/Shaders/screen.fs");
    LoadSimpleShader(GL,
                     "../Data/Shaders/simple.vs",
                     "../Data/Shaders/simple.fs");
    LoadGuiRectShader(GL,
                      "../Data/Shaders/gui_rect.vs",
                      "../Data/Shaders/gui_rect.fs");
    LoadGuiGeomShader(GL,
                      "../Data/Shaders/gui_geom.vs",
                      "../Data/Shaders/gui_geom.fs");
    LoadLinesShader(GL,
                    "../Data/Shaders/lines.vs",
                    "../Data/Shaders/lines.fs");
    LoadGuiLinesShader(GL,
                       "../Data/Shaders/gui_geom_lines.vs",
                       "../Data/Shaders/gui_geom_lines.fs");
    LoadResolveShader(GL,
                      "../Data/Shaders/screen.vs",
                      "../Data/Shaders/resolve_gbuf_texture.fs");
    
    LoadLightingShader(GL,
                       "../Data/Shaders/lighting.vs",
                       "../Data/Shaders/lighting.fs");
    
    LoadSSAOShader(GL,
                   "../Data/Shaders/ssao.vs",
                   "../Data/Shaders/ssao.fs");
    
    LoadFilterShader(GL,
                     "../Data/Shaders/screen.vs",
                     "../Data/Shaders/kernel_filter.fs");
    
    size_t FS = sizeof(float);
    
    // NOTE(Dima): Initializing screen rect
    float screenFaceVerts[] = {
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };
    
    glGenVertexArrays(1, &GL->ScreenVAO);
    glGenBuffers(1, &GL->ScreenVBO);
    
    glBindVertexArray(GL->ScreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->ScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenFaceVerts), screenFaceVerts, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, 0, 4 * FS, 0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Initializing GuiRect mesh
    glGenVertexArrays(1, &GL->GuiRectVAO);
    glGenBuffers(1, &GL->GuiRectVBO);
    
    glBindVertexArray(GL->GuiRectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiRectVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 8 * FS, 0, GL_DYNAMIC_DRAW);
    
    if(ArrayIsValid(GL->GuiRectShader.PUVAttrLoc)){
        glEnableVertexAttribArray(GL->GuiRectShader.PUVAttrLoc);
        glVertexAttribPointer(0, 4, GL_FLOAT, 0, 8 * FS, 0);
    }
    
    if(ArrayIsValid(GL->GuiRectShader.CAttrLoc)){
        glEnableVertexAttribArray(GL->GuiRectShader.CAttrLoc);
        glVertexAttribPointer(1, 4, GL_FLOAT, 0, 8 * FS, (void*)(4 * FS));
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Init LargeAtlas bitmap
    GL->LargeAtlasHandle = GlAllocateTexture(&Assets->MainLargeAtlas.Bitmap);
    
    // NOTE(Dima): Binding gui shader texture to 0 unit
    UseShader(&GL->GuiRectShader.Shader);
    glUniform1i(GL->GuiRectShader.BitmapLoc, 0);
    glUseProgram(0);
    
    // NOTE(Dima): Init lines geometry buffer objects
    glGenVertexArrays(1, &GL->LinesVAO);
    glGenBuffers(2, GL->LinesVBO);
    glGenBuffers(2, GL->LinesTBO);
    
    // NOTE(Dima): Init GuiGeom buffer objects
    glGenVertexArrays(1, &GL->GuiGeomVAO);
    glGenBuffers(1, &GL->GuiGeomVBO);
    glGenBuffers(1, &GL->GuiGeomEBO);
    glGenBuffers(1, &GL->GuiGeomTB);
    
    glBindVertexArray(GL->GuiGeomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiGeomVBO);
    InitVertexAttribFloat(GL->GuiGeomShader.PUVAttrLoc,
                          4, 8 * FS, 0);
    InitVertexAttribFloat(GL->GuiGeomShader.CAttrLoc,
                          4, 8 * FS, 4 * FS);
    glBindVertexArray(0);
    
    // NOTE(Dima): Init gui lines geometry
    glGenVertexArrays(1, &GL->GuiLinesVAO);
    glGenBuffers(1, &GL->GuiLinesVBO);
    glGenBuffers(1, &GL->GuiLinesColorsTBO);
    
    glBindVertexArray(GL->GuiLinesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiLinesVBO);
    InitVertexAttribFloat(GL->GuiLinesShader.PAttrLoc,
                          2, 2 * FS, 0);
    glBindVertexArray(0);
    
    
    // NOTE(Dima): Init GBuffer textures & framebuffers
    glGenFramebuffers(1, &GL->GBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, GL->GBuffer);
    
    glGenTextures(1, &GL->GAlbedoSpecTex);
    glBindTexture(GL_TEXTURE_2D, GL->GAlbedoSpecTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
                 Render->InitWindowWidth, 
                 Render->InitWindowHeight, 
                 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, GL->GAlbedoSpecTex, 0);
    
    glGenTextures(1, &GL->GNormalMetalRoughTex);
    glBindTexture(GL_TEXTURE_2D, GL->GNormalMetalRoughTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 
                 Render->InitWindowWidth, 
                 Render->InitWindowHeight, 
                 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 
                           GL_TEXTURE_2D, GL->GNormalMetalRoughTex, 0);
    
    // NOTE(Dima): Init Gbuffer attachements
    GLuint GBufferAttachments[2];
    GBufferAttachments[0] = GL_COLOR_ATTACHMENT0;
    GBufferAttachments[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, GBufferAttachments);
    
    // NOTE(Dima): Init Gbuffer depth
    glGenTextures(1, &GL->GBufferDepthTex);
    glBindTexture(GL_TEXTURE_2D, GL->GBufferDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, 
                 GL_DEPTH_COMPONENT32, 
                 Render->InitWindowWidth,
                 Render->InitWindowHeight,
                 0, GL_DEPTH_COMPONENT, 
                 GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_DEPTH_ATTACHMENT, 
                           GL_TEXTURE_2D, 
                           GL->GBufferDepthTex, 0);
    
    CheckFramebufferStatus();
    
    // NOTE(Dima): Init SSAO textures & framebuffers
    
#if 1
    glGenFramebuffers(1, &GL->SSAO_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, GL->SSAO_FBO);
    glGenTextures(1, &GL->SSAO_Tex);
    glBindTexture(GL_TEXTURE_2D, GL->SSAO_Tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 
                 Render->InitWindowWidth,
                 Render->InitWindowHeight,
                 0, GL_RED, GL_FLOAT, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, GL->SSAO_Tex, 0);
    
    CheckFramebufferStatus();
    
    glGenFramebuffers(1, &GL->SSAOBlur_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, GL->SSAOBlur_FBO);
    glGenTextures(1, &GL->SSAOBlur_Tex);
    glBindTexture(GL_TEXTURE_2D, GL->SSAOBlur_Tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 
                 Render->InitWindowWidth,
                 Render->InitWindowHeight,
                 0, GL_RED, GL_FLOAT, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                           GL_TEXTURE_2D, GL->SSAOBlur_Tex, 0);
    
    CheckFramebufferStatus();
    
    glGenBuffers(1, &GL->SSAOKernelBuf);
    glBindBuffer(GL_TEXTURE_BUFFER, GL->SSAOKernelBuf);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(Render->SSAOKernelSamples), 
                 Render->SSAOKernelSamples, GL_STATIC_DRAW);
    
    glGenTextures(1, &GL->SSAOKernelTex);
    glBindTexture(GL_TEXTURE_BUFFER, GL->SSAOKernelTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, GL->SSAOKernelBuf);
    
    glGenTextures(1, &GL->SSAONoiseTex);
    glBindTexture(GL_TEXTURE_2D, GL->SSAONoiseTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 
                 GL_RGB32F,
                 4, 4, 0,
                 GL_RGB, GL_FLOAT,
                 Render->SSAONoiseTexture);
#endif
    
}

INTERNAL_FUNCTION void GlFree(gl_state* GL){
    // NOTE(Dima): Freeing all of the shader programs
    for(int ProgramIndex = 0;
        ProgramIndex < GL->ProgramsCount;
        ProgramIndex++)
    {
        glDeleteProgram(GL->Programs[ProgramIndex].ID);
    }
    
    // NOTE(Dima): Deleting main large atlas texture
    glDeleteTextures(1, &GL->LargeAtlasHandle);
    
    // NOTE(Dima): Free GuiGeom buffer objects
    glDeleteBuffers(1, &GL->GuiGeomEBO);
    glDeleteBuffers(1, &GL->GuiGeomVBO);
    glDeleteBuffers(1, &GL->GuiGeomTB);
    glDeleteVertexArrays(1, &GL->GuiGeomVAO);
    
    // NOTE(Dima): Deleting render framebuffer
    glDeleteFramebuffers(1, &GL->GBuffer);
    glDeleteTextures(1, &GL->GBufferDepthTex);
    glDeleteTextures(1, &GL->GNormalMetalRoughTex);
    glDeleteTextures(1, &GL->GAlbedoSpecTex);
    
    // NOTE(Dima): Deleting for SSAO
    glDeleteFramebuffers(1, &GL->SSAO_FBO);
    glDeleteTextures(1, &GL->SSAO_Tex);
    
    glDeleteFramebuffers(1, &GL->SSAOBlur_FBO);
    glDeleteTextures(1, &GL->SSAOBlur_Tex);
    
    glDeleteTextures(1, &GL->SSAONoiseTex);
    glDeleteTextures(1, &GL->SSAOKernelTex);
    glDeleteBuffers(1, &GL->SSAOKernelBuf);
}

INTERNAL_FUNCTION void GlRenderGuiRect(gl_state* GL,
                                       float* GuiOrtho,
                                       b32 BitmapSetFlag,
                                       GLuint BmpHandle,
                                       float* RectArr,
                                       size_t RectArrSize)
{
    UseShader(&GL->GuiRectShader.Shader);
    glUniform1i(GL->GuiRectShader.BitmapIsSetLoc, BitmapSetFlag);
    
    if(BitmapSetFlag == 1){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BmpHandle);
        glUniform1i(GL->GuiRectShader.BitmapLoc, 0);
    }
    
    glUniformMatrix4fv(GL->GuiRectShader.ProjectionLoc,
                       1, GL_TRUE,
                       GuiOrtho);
    
    glBindVertexArray(GL->GuiRectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiRectVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, RectArrSize, RectArr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    if(BitmapSetFlag == 1){
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    glUseProgram(0);
}


INTERNAL_FUNCTION void GlShowDynamicBitmap(gl_state* GL, bmp_info* bmp){
    
    // NOTE(Dima): Blit texture load
    GLuint BlitTex;
    glGenTextures(1, &BlitTex);
    glBindTexture(GL_TEXTURE_2D, BlitTex);
    glTexImage2D(
                 GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 bmp->Width,
                 bmp->Height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 bmp->Pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // NOTE(Dima): Drawing screen rect
    glBindVertexArray(GL->ScreenVAO);
    glUseProgram(GL->ScreenShader.Shader.ID);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlitTex);
    glUniform1i(GL->ScreenShader.ScreenTextureLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glUseProgram(0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Freeing blit texture
    glDeleteTextures(1, &BlitTex);
}

void GlOutputPass(gl_state* GL, render_state* Render, 
                  void* Begin, void* EndExcl,
                  u32 PassType,
                  render_camera_setup* CameraSetup)
{
    //u8* at = (u8*)Render->StackRegion.CreationBlock.Base;
    //u8* StackEnd = (u8*)Render->StackRegion.CreationBlock.Base + Render->StackRegion.CreationBlock.Used;
    
    b32 QueueBeginned = false;
    int BeginnedQueueIndex = RENDER_DEFAULT_QUEUE_INDEX;
    
    u8* At = (u8*)Begin;
    
    while (At < (u8*)EndExcl) {
        render_entry_header* Header = (render_entry_header*)At;
        
        void* AtBeforeInc = At;
        At += sizeof(render_entry_header);
        
        switch(Header->Type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(render_entry_clear_color);
                
                glClearColor(Entry->clearColor01.r,
                             Entry->clearColor01.g,
                             Entry->clearColor01.b,
                             1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }break;
            
            case RenderEntry_BeginQueue:{
                RENDER_GET_ENTRY(render_entry_renderqueue);
                
                QueueBeginned = true;
                BeginnedQueueIndex = Entry->QueueIndex;
                
                render_queue* Queue = &Render->Queues[Entry->QueueIndex];
                
                Queue->FirstEntry = 0;
                Queue->OnePastLastEntry = 0;
            }break;
            
            case RenderEntry_EndQueue:{
                RENDER_GET_ENTRY(render_entry_renderqueue);
                
                Assert(Entry->QueueIndex == BeginnedQueueIndex);
                QueueBeginned = false;
                BeginnedQueueIndex = RENDER_DEFAULT_QUEUE_INDEX;
                
                render_queue* Queue = &Render->Queues[Entry->QueueIndex];
                Queue->OnePastLastEntry = AtBeforeInc;
            }break;
            
            case RenderEntry_RenderPass:{
                RENDER_GET_ENTRY(render_entry_renderpass);
                
                if(!RenderQueueIsEmpty(Render, Entry->QueueIndex)){
                    
                    render_queue* Queue = &Render->Queues[Entry->QueueIndex];
                    render_camera_setup* PassCamSetup = &Render->CameraSetups[Entry->CameraSetupIndex];
                    
                    GlOutputPass(GL, Render, 
                                 Queue->FirstEntry, 
                                 Queue->OnePastLastEntry,
                                 RenderPass_RenderPassEntry,
                                 PassCamSetup);
                }
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(render_entry_bitmap);
                
                if(!Entry->Bitmap->Handle){
                    GlAllocateTexture(Entry->Bitmap);
                }
                
                v2 Min = Entry->P;
                v2 Max = V2(Min.x + Entry->Bitmap->WidthOverHeight * Entry->PixelHeight,
                            Min.y + Entry->PixelHeight);
                
                float r = Entry->ModulationColor01.r;
                float g = Entry->ModulationColor01.g;
                float b = Entry->ModulationColor01.b;
                float a = Entry->ModulationColor01.a;
                
                float RectArr[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Max.y, 1.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    Min.x, Min.y, 0.0f, 0.0f, r, g, b, a,
                };
                
                GlRenderGuiRect(GL, GL->GuiOrtho.e, true, 
                                Entry->Bitmap->Handle,
                                RectArr, sizeof(RectArr));
                
            }break;
            
            case RenderEntry_Mesh:{
                if(ProcessQueueEntry(Render, 
                                     BeginnedQueueIndex,
                                     PassType, 
                                     AtBeforeInc))
                {
                    RENDER_GET_ENTRY(render_entry_mesh);
                    
                    mesh_info* Mesh = Entry->Mesh;
                    
                    GlAllocateMesh(GL, Mesh);
                    
                    glEnable(GL_DEPTH_TEST);
                    
                    
                    
                    UseShader(&GL->SimpleShader.Shader);
                    
                    // NOTE(Dima): Setting VS uniforms
                    UniformMatrix4x4(GL->SimpleShader.ModelLoc,
                                     Entry->Transform.e);
                    UniformMatrix4x4(GL->SimpleShader.ViewLoc,
                                     CameraSetup->View.e);
                    UniformMatrix4x4(GL->SimpleShader.ProjectionLoc,
                                     CameraSetup->Projection.e);
                    UniformBool(GL->SimpleShader.HasSkinningLoc,
                                Mesh->TypeCtx.MeshType == Mesh_Skinned);
                    UniformInt(GL->SimpleShader.BonesCountLoc, 
                               Entry->BoneCount);
                    
                    if(Entry->BoneCount){
                        glUniformMatrix4fv(GL->SimpleShader.BoneTransformsLoc,
                                           Entry->BoneCount,
                                           GL_TRUE,
                                           (const GLfloat*)Entry->BoneTransforms[0].e);
                    }
                    
                    //UniformInt(GL->SimpleShader.);
                    
                    // NOTE(Dima): Setting FS uniforms
                    b32 AlbedoIsSet = 0;
                    b32 NormalsIsSet = 0;
                    b32 SpecularIsSet = 0;
                    b32 EmissiveIsSet = 0;
                    
                    if(Entry->Material){
                        material_info* Mat = Entry->Material;
                        
                        bmp_info* Albedo = Mat->Textures[MaterialTexture_Diffuse];
                        bmp_info* Normals = Mat->Textures[MaterialTexture_Normals];
                        bmp_info* Specular = Mat->Textures[MaterialTexture_Specular];
                        bmp_info* Emissive = Mat->Textures[MaterialTexture_Emissive];
                        
                        
                        if(Albedo){
                            AlbedoIsSet = 1;
                            GlAllocateTexture(Albedo);
                            
                            glUniform1i(GL->SimpleShader.AlbedoLoc, 0);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, Albedo->Handle);
                        }
                        
                        if(Specular){
                            SpecularIsSet = 1;
                            GlAllocateTexture(Specular);
                        }
                        
                        if(Normals){
                            NormalsIsSet = 1;
                            GlAllocateTexture(Normals);
                            
                            glUniform1i(GL->SimpleShader.NormalsLoc, 1);
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, Normals->Handle);
                        }
                        
                        if(Emissive){
                            EmissiveIsSet = 1;
                            GlAllocateTexture(Emissive);
                        }
                    }
                    
                    u32 TexturesSetFlags = (AlbedoIsSet |
                                            (NormalsIsSet << 1) |
                                            (SpecularIsSet << 2) |
                                            (EmissiveIsSet << 3));
                    
                    UniformVec3(GL->SimpleShader.AlbedoColorLoc, Entry->AlbedoColor);
                    UniformInt(GL->SimpleShader.TexturesSetFlagsLoc, TexturesSetFlags);
                    
                    glBindVertexArray(Mesh->Handles.Handles[0]);
                    glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
                    
                    glDisable(GL_DEPTH_TEST);
                    //GlFreeMeshHandles(&Mesh->Handles);
                }
            }break;
        }
        
        At += Header->DataSize;
    }
}

INTERNAL_FUNCTION void OutputLinesArray(gl_state* GL,
                                        render_line_primitive* Lines,
                                        v3* LineColors, int Count,
                                        GLuint LinesVBO,
                                        GLuint ColorsTBO)
{
    if(Count){
        BindBufferAndFill(GL_ARRAY_BUFFER,
                          GL_DYNAMIC_DRAW,
                          Lines,
                          Count * sizeof(render_line_primitive),
                          LinesVBO);
        
        InitVertexAttribFloat(GL->LinesShader.PAttrLoc,
                              3, 3 * sizeof(float), 0);
        
        BindBufferAndFill(GL_TEXTURE_BUFFER,
                          GL_DYNAMIC_DRAW,
                          LineColors,
                          Count * sizeof(v3),
                          ColorsTBO);
        
        GLuint ColorsTex = GenerateAndBindBufferTexture(GL_TEXTURE0,
                                                        GL_RGB32F,
                                                        ColorsTBO);
        
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, Count * 2);
        
        FreeVertexAttrib(GL->LinesShader.PAttrLoc);
        
        glDeleteTextures(1, &ColorsTex);
    }
}

INTERNAL_FUNCTION void OutputRenderLines(gl_state* GL,
                                         render_state* Render,
                                         const m44& ViewProjection)
{
    // NOTE(Dima): Binding array object
    glBindVertexArray(GL->LinesVAO);
    
    UseShader(&GL->LinesShader.Shader);
    UniformMatrix4x4(GL->LinesShader.ViewProjectionLoc,
                     (float*)ViewProjection.e);
    glUniform1i(GL->LinesShader.ColorsTextureLoc, 0);
    
    // NOTE(Dima): Outputing with depth lines
    glEnable(GL_DEPTH_TEST);
    render_lines_chunk* ChunkAt = Render->LinesGeom.FirstDepth;
    while(ChunkAt){
        
        OutputLinesArray(GL, 
                         ChunkAt->Lines,
                         ChunkAt->Colors,
                         ChunkAt->Count,
                         GL->LinesVBO[0],
                         GL->LinesTBO[0]);
        
        ChunkAt = ChunkAt->Next;
    }
    
    // NOTE(Dima): Outputing no-depth lines
    glDisable(GL_DEPTH_TEST);
    ChunkAt = Render->LinesGeom.FirstNoDepth;
    while(ChunkAt){
        OutputLinesArray(GL, 
                         ChunkAt->Lines,
                         ChunkAt->Colors,
                         ChunkAt->Count,
                         GL->LinesVBO[1],
                         GL->LinesTBO[1]);
        
        ChunkAt = ChunkAt->Next;
    }
}

INTERNAL_FUNCTION void OutputGuiGeometry(gl_state* GL, 
                                         render_state* Render)
{
    glBindVertexArray(GL->GuiGeomVAO);
    BindBufferAndFill(GL_ARRAY_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Render->GuiGeom.Vertices,
                      Render->GuiGeom.VerticesCount * sizeof(render_gui_geom_vertex),
                      GL->GuiGeomVBO);
    
    BindBufferAndFill(GL_ELEMENT_ARRAY_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Render->GuiGeom.Indices,
                      Render->GuiGeom.IndicesCount * sizeof(u32),
                      GL->GuiGeomEBO);
    
    BindBufferAndFill(GL_TEXTURE_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Render->GuiGeom.TriangleGeomTypes,
                      Render->GuiGeom.TriangleGeomTypesCount * sizeof(u8),
                      GL->GuiGeomTB);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL->LargeAtlasHandle);
    
    // NOTE(Dima): Passing geom types
    GLuint TexBufTex = GenerateAndBindBufferTexture(GL_TEXTURE1, GL_R8I, GL->GuiGeomTB);
    
    UseShader(&GL->GuiGeomShader.Shader);
    UniformMatrix4x4(GL->GuiGeomShader.ProjectionLoc, GL->GuiOrtho.e);
    
    // NOTE(Dima): Passing Bitmap uniforms to shader
    glUniform1i(GL->GuiGeomShader.BitmapLoc, 0);
    glUniform1i(GL->GuiGeomShader.TriangleGeomTypesLoc, 1);
    
    for(int ChunkIndex = 0;
        ChunkIndex < Render->GuiGeom.CurChunkIndex;
        ChunkIndex++)
    {
        render_gui_chunk* CurChunk = &Render->GuiGeom.Chunks[ChunkIndex];
        
        if(CurChunk->IndicesCount){
            v2 ClipDim = GetRectDim(CurChunk->ClipRect);
            rc2 ClipRect = BottomLeftToTopLeftRectange(CurChunk->ClipRect,
                                                       Render->FrameInfo.Height);
            
            
            glScissor(ClipRect.Min.x,
                      ClipRect.Min.y,
                      ClipDim.x,
                      ClipDim.y);
            
            glDrawElementsBaseVertex(GL_TRIANGLES,
                                     CurChunk->IndicesCount,
                                     GL_UNSIGNED_INT,
                                     0,
                                     CurChunk->BaseVertex);
        }
    }
    
    glBindVertexArray(0);
    
    glDeleteTextures(1, &TexBufTex);
}

INTERNAL_FUNCTION void OutputGuiGeometryLines(gl_state* GL, render_state* Render){
    glBindVertexArray(GL->GuiLinesVAO);
    
    BindBufferAndFill(GL_ARRAY_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Render->GuiGeom.LinePoints,
                      Render->GuiGeom.LinePointsCount * sizeof(v2),
                      GL->GuiLinesVBO);
    
    BindBufferAndFill(GL_TEXTURE_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Render->GuiGeom.LineColors,
                      Render->GuiGeom.LineColorsCount * sizeof(v4),
                      GL->GuiLinesColorsTBO);
    
    UseShader(&GL->GuiLinesShader.Shader);
    UniformMatrix4x4(GL->GuiLinesShader.ProjectionLoc,
                     (float*)GL->GuiOrtho.e);
    glUniform1i(GL->GuiLinesShader.ColorsTextureLoc, 0);
    
    GLuint ColorsTex = GenerateAndBindBufferTexture(GL_TEXTURE0,
                                                    GL_RGBA32F,
                                                    GL->GuiLinesColorsTBO);
    glLineWidth(1.0f);
    
    for(int ChunkIndex = 0;
        ChunkIndex < Render->GuiGeom.CurChunkIndex;
        ChunkIndex++)
    {
        render_gui_chunk* CurChunk = &Render->GuiGeom.Chunks[ChunkIndex];
        
        if(CurChunk->LinePointsCount){
            
            v2 ClipDim = GetRectDim(CurChunk->ClipRect);
            rc2 ClipRect = BottomLeftToTopLeftRectange(CurChunk->ClipRect,
                                                       Render->FrameInfo.Height);
            
            glScissor(ClipRect.Min.x,
                      ClipRect.Min.y,
                      ClipDim.x,
                      ClipDim.y);
            
            glDrawArrays(GL_LINES, 
                         CurChunk->LinePointsBase, 
                         CurChunk->LinePointsCount);
        }
    }
    
    glDeleteTextures(1, &ColorsTex);
}

INTERNAL_FUNCTION void ApplyFilter(gl_state* GL, render_state* Render,
                                   GLuint TargetFBO,
                                   GLuint TextureToFilter,
                                   u32 FilterType)
{
    b32 IsValid = true;
    float* Target = 0;
    int TargetCount = 0;
    switch(FilterType)
    {
        case RenderFilter_GaussianBlur5x5:
        {
            Target = Render->GaussianBlur5;
            TargetCount = 25;
        }break;
        
        case RenderFilter_GaussianBlur3x3:
        {
            Target = Render->GaussianBlur3;
            TargetCount = 9;
        }break;
        
        case RenderFilter_BoxBlur5x5:
        {
            Target = &Render->BoxBlur5;
            TargetCount = 1;
        }break;
        
        case RenderFilter_BoxBlur3x3:
        {
            Target = &Render->BoxBlur3;
            TargetCount = 1;
        }break;
        
        default:
        {
            IsValid = false;
        }break;
    }
    
    if(IsValid)
    {
        gl_filter_shader* FilterSh = &GL->FilterShader;
        glBindFramebuffer(GL_FRAMEBUFFER, TargetFBO);
        
        glUseProgram(FilterSh->Shader.ID);
        glUniform1i(FilterSh->UVInvertYLoc, true);
        UniformTexture2D(FilterSh->InputTexLoc,
                         TextureToFilter, 0);
        glUniform1fv(FilterSh->FilterLoc,
                     TargetCount,
                     Target);
        glUniform1i(FilterSh->FilterTypeLoc,
                    FilterType);
        
        glBindVertexArray(GL->ScreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
    }
}

INTERNAL_FUNCTION void GlFinalOutput(gl_state* GL, render_state* Render){
    render_frame_info FrameInfo = Render->FrameInfo;
    
    render_camera_setup* CamSet = &Render->CameraSetups[0];
    
    // NOTE(Dima): Rendering SSAO
    {
        BLOCK_TIMING("SSAO Compute");
        
        gl_ssao_shader* ShaderSSAO = &GL->SSAOShader;
        glBindFramebuffer(GL_FRAMEBUFFER, GL->SSAO_FBO);
        
        glUseProgram(ShaderSSAO->Shader.ID);
        glUniform1i(ShaderSSAO->UVInvertYLoc, true);
        glUniform2f(ShaderSSAO->WidthHeightLoc, 
                    CamSet->FramebufferWidth, 
                    CamSet->FramebufferHeight);
        glUniform2f(ShaderSSAO->FarNearLoc, CamSet->Far, CamSet->Near);
        glUniform1f(ShaderSSAO->FOVRadiansLoc, CamSet->FOVRadians);
        
        v4 Coefs;
        Coefs.x = CamSet->Projection.e[0];
        Coefs.y = CamSet->Projection.e[5];
        Coefs.z = CamSet->Projection.e[10];
        Coefs.w = CamSet->Projection.e[14];
        UniformVec4(ShaderSSAO->PerspProjCoefsLoc, Coefs);
        
        UniformTexture2D(ShaderSSAO->DepthTexLoc,
                         GL->GBufferDepthTex, 0);
        
        UniformTexture2D(ShaderSSAO->NormalMetalRoughTexLoc, 
                         GL->GNormalMetalRoughTex, 1);
        
        UniformTexture2D(ShaderSSAO->SSAONoiseTexLoc,
                         GL->SSAONoiseTex, 2);
        
        UniformTextureBuffer(ShaderSSAO->SSAOKernelBufLoc,
                             GL->SSAOKernelTex, 3);
        
        glUniform1i(ShaderSSAO->SSAOKernelSamplesCountLoc, Render->SSAOKernelSampleCount);
        glUniform1f(ShaderSSAO->SSAOKernelRadiusLoc, Render->SSAOKernelRadius);
        glUniform1f(ShaderSSAO->SSAOContributionLoc, Render->SSAOContribution);
        glUniform1f(ShaderSSAO->SSAORangeCheckLoc, Render->SSAORangeCheck);
        
        glBindVertexArray(GL->ScreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glUseProgram(0);
        glBindVertexArray(0);
    }
    
    // NOTE(Dima): Rendering SSAO blur
    {
        BLOCK_TIMING("SSAO Blur");
        
        ApplyFilter(GL, Render, 
                    GL->SSAOBlur_FBO,
                    GL->SSAO_Tex,
                    Render->SSAOFilterType);
    }
    
    //glBindFramebuffer(GL_FRAMEBUFFER, GL->RenderFBO);
    
    // NOTE(Dima): outputing lines
    OutputRenderLines(GL, Render, 
                      Render->CameraSetups[0].ViewProjection);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // NOTE(Dima): Common attributes for Resolve and GBuffer shaders
    b32 IsGReconstruction = false;
    GLint FarNearLoc;
    GLint UVInvertYLoc;
    GLuint ProgramID;
    
    gl_lighting_shader* LitSh = &GL->LightingShader;
    gl_resolve_shader* SolveSh = &GL->ResolveShader;
    
    if(Render->ToShowBufferType == RenderShowBuffer_Main)
    {
        IsGReconstruction = true;
        FarNearLoc = LitSh->FarNearLoc;
        UVInvertYLoc = LitSh->UVInvertYLoc;
        ProgramID = LitSh->Shader.ID;
    }
    else{
        IsGReconstruction = false;
        FarNearLoc = SolveSh->FarNearLoc;
        UVInvertYLoc = SolveSh->UVInvertYLoc;
        ProgramID = SolveSh->Shader.ID;
    }
    
    glViewport(0.0f, 0.0f, 
               FrameInfo.Width, 
               FrameInfo.Height);
    
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // NOTE(Dima): Drawing screen rect
    glBindVertexArray(GL->ScreenVAO);
    glUseProgram(ProgramID);
    
    if(ProgramID == SolveSh->Shader.ID)
    {
        glUniform1i(SolveSh->TextureResolveTypeLoc, Render->ToShowBufferType);
        
        GLuint TexToResolve = 0;
        
        if((Render->ToShowBufferType == RenderShowBuffer_Albedo) ||
           (Render->ToShowBufferType == RenderShowBuffer_Specular))
        {
            TexToResolve = GL->GAlbedoSpecTex;
        }
        else if(Render->ToShowBufferType == RenderShowBuffer_Depth)
        {
            TexToResolve = GL->GBufferDepthTex;
        }
        else if((Render->ToShowBufferType == RenderShowBuffer_Normal) ||
                (Render->ToShowBufferType == RenderShowBuffer_Metal) ||
                (Render->ToShowBufferType == RenderShowBuffer_Roughness))
        {
            TexToResolve = GL->GNormalMetalRoughTex;
        }
        else if(Render->ToShowBufferType == RenderShowBuffer_SSAO)
        {
            TexToResolve = GL->SSAO_Tex;
        }
        else if(Render->ToShowBufferType == RenderShowBuffer_SSAOBlur)
        {
            TexToResolve = GL->SSAOBlur_Tex;
        }
        
        UniformTexture2D(SolveSh->TextureToResolveLoc,
                         TexToResolve, 0);
    }
    else
    {
        Assert(IsGReconstruction);
        
        UniformFloat(LitSh->AspectRatioLoc, CamSet->AspectRatio);
        UniformFloat(LitSh->FOVRadiansLoc, CamSet->FOVRadians);
        
        UniformTexture2D(LitSh->GNormalMetalRoughLoc, 
                         GL->GNormalMetalRoughTex, 0);
        
        UniformTexture2D(LitSh->GAlbedoSpecLoc,
                         GL->GAlbedoSpecTex, 1);
        
        UniformTexture2D(LitSh->GDepthTexLoc,
                         GL->GBufferDepthTex, 2);
        
        UniformTexture2D(LitSh->SSAOInputLoc,
                         GL->SSAOBlur_Tex, 3);
        
        UniformBool(LitSh->FogEnabledLoc, Render->FogEnabled);
        UniformFloat(LitSh->FogGradientLoc, Render->FogGradient);
        UniformFloat(LitSh->FogDensityLoc, Render->FogDensity);
        UniformVec3(LitSh->FogColorLoc, Render->FogColor);
    }
    
    UniformVec2(FarNearLoc, CamSet->Far, CamSet->Near);
    UniformBool(UVInvertYLoc, true);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glUseProgram(0);
    glBindVertexArray(0);
    
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    
    // NOTE(Dima): Outputing gui geometry
    glEnable(GL_SCISSOR_TEST);
    OutputGuiGeometry(GL, Render);
    OutputGuiGeometryLines(GL, Render);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

INTERNAL_FUNCTION void GlOutputRender(gl_state* GL, render_state* Render){
    FUNCTION_TIMING();
    
    // NOTE(Dima): Calculating gui orthographic projection matrix
    Assert(Render->FrameInfoIsSet);
    render_frame_info FrameInfo = Render->FrameInfo;
    
    float a = 2.0f / (float)FrameInfo.InitWidth;
    float b = 2.0f / (float)FrameInfo.InitHeight;
    
    GLfloat GuiOrtho[16] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, -b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    GL->GuiOrtho = Floats2Matrix(GuiOrtho);
    
    glBindFramebuffer(GL_FRAMEBUFFER, GL->GBuffer);
    glViewport(0.0f, 0.0f, 
               FrameInfo.InitWidth, 
               FrameInfo.InitHeight);
    
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    GlOutputPass(GL, Render,
                 Render->StackRegion.CreationBlock.Base,
                 (void*)((u8*)Render->StackRegion.CreationBlock.Base + Render->StackRegion.CreationBlock.Used),
                 RenderPass_Main,
                 &Render->CameraSetups[0]);
    
    GlFinalOutput(GL, Render);
}
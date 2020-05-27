#include "joy_opengl.h"

#include <assert.h>

INTERNAL_FUNCTION GLuint GlLoadFromSource(char* VertexSource, char* FragmentSource, char* GeometrySource = 0) {
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
        fprintf(stderr, "Error while loading vertex shader(%s)\n%s\n", VertexSource, InfoLog);
        Platform.OutputString(InfoLog);
        assert(Success);
    }
    
	FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentSource, 0);
	glCompileShader(FragmentShader);
    
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(FragmentShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        fprintf(stderr, "Error while loading fragment shader(%s)\n%s\n", FragmentSource, InfoLog);
        Platform.OutputString(InfoLog);
        assert(Success);
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
                    GeometrySource, InfoLog);
            Platform.OutputString(InfoLog);
            assert(Success);
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
        assert(Success);
	}
    
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
    if(GeometrySource){
        glDeleteShader(GeometryShader);
    }
    
	return(Program);
}

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
        assert(Success);
    }
    
    
	GLuint Program = glCreateProgram();
	glAttachShader(Program, ComputeShader);
    glLinkProgram(Program);
    
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(Program, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        Platform.OutputString(InfoLog);
        assert(Success);
	}
    
	glDeleteShader(ComputeShader);
    
	return(Program);
}

INTERNAL_FUNCTION gl_shader GlLoadProgram(gl_state* GL, char* vertexPath, char* fragmentPath, char* geometryPath = 0) {
    assert(GL->ProgramsCount < ARRAY_COUNT(GL->Programs));
	int resultIndex = GL->ProgramsCount;
    gl_program* result = GL->Programs + GL->ProgramsCount++;
    
    gl_shader ResultShader = {};
    
	Platform_Read_File_Result vFile = Platform.ReadFile(vertexPath);
	Platform_Read_File_Result fFile = Platform.ReadFile(fragmentPath);
    Platform_Read_File_Result gFile = {};
    char* toPassGeometryData = 0;
    if(geometryPath){
        gFile = Platform.ReadFile(geometryPath);
        toPassGeometryData = (char*)gFile.data;
    }
    
	result->ID = GlLoadFromSource(
                                  (char*)vFile.data, 
                                  (char*)fFile.data, 
                                  toPassGeometryData);
    
    ResultShader.ID = result->ID;
    ResultShader.Type = GlShader_Simple;
    ResultShader._InternalProgramIndex = resultIndex;
    
	Platform.FreeFileMemory(&vFile);
	Platform.FreeFileMemory(&fFile);
    if(geometryPath){
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

void UniformMatrix4x4(GLint Loc, float* Data){
    glUniformMatrix4fv(Loc, 1, true, Data);
}

void UniformMatrixArray4x4(GLint Loc, int Count, m44* Array){
    glUniformMatrix4fv(Loc, Count, true, (const GLfloat*)Array);
}


#define LOAD_SHADER_FUNC(name) void name(gl_state* GL, char* PathV, char* PathF)

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadScreenShader){
    gl_screen_shader& Result = GL->ScreenShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(ScreenTexture);
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
                              assets* Assets)
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
    
    // NOTE(Dima): Init SSAO textures
    glGenTextures(1, &GL->SSAONoiseTex);
    glGenTextures(1, &GL->SSAOKernelTex);
    
    glBindTexture(GL_TEXTURE_2D, GL->SSAOKernelTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 
                 GL_RGB32F,
                 Render->SSAOKernelSampleCount, 1, 0,
                 GL_RGB, GL_FLOAT,
                 Render->SSAOKernelSamples);
    
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
    
    glBindVertexArray(0);
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

void GlOutputStack(gl_state* GL, render_stack* Stack, render_camera_setup* CameraSetup){
    u8* at = (u8*)Stack->MemRegion.CreationBlock.Base;
    u8* StackEnd = (u8*)Stack->MemRegion.CreationBlock.Base + Stack->MemRegion.CreationBlock.Used;
    
    while (at < StackEnd) {
        render_entry_header* Header = (render_entry_header*)at;
        
        at += sizeof(render_entry_header);
        
        switch(Header->type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(render_entry_clear_color);
                
                glClearColor(entry->clearColor01.r,
                             entry->clearColor01.g,
                             entry->clearColor01.b,
                             1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(render_entry_bitmap);
                
                if(!entry->Bitmap->Handle){
                    GlAllocateTexture(entry->Bitmap);
                }
                
                v2 Min = entry->P;
                v2 Max = V2(Min.x + entry->Bitmap->WidthOverHeight * entry->PixelHeight,
                            Min.y + entry->PixelHeight);
                
                float r = entry->ModulationColor01.r;
                float g = entry->ModulationColor01.g;
                float b = entry->ModulationColor01.b;
                float a = entry->ModulationColor01.a;
                
                float RectArr[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Max.y, 1.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    Min.x, Min.y, 0.0f, 0.0f, r, g, b, a,
                };
                
                GlRenderGuiRect(GL, GL->GuiOrtho.e, true, 
                                entry->Bitmap->Handle,
                                RectArr, sizeof(RectArr));
                
            }break;
            
            case RenderEntry_Mesh:{
                RENDER_GET_ENTRY(render_entry_mesh);
                mesh_info* Mesh = entry->Mesh;
                
                GlAllocateMesh(GL, Mesh);
                
                glEnable(GL_DEPTH_TEST);
                
                UseShader(&GL->SimpleShader.Shader);
                
                // NOTE(Dima): Setting VS uniforms
                UniformMatrix4x4(GL->SimpleShader.ModelLoc,
                                 entry->Transform.e);
                UniformMatrix4x4(GL->SimpleShader.ViewLoc,
                                 CameraSetup->View.e);
                UniformMatrix4x4(GL->SimpleShader.ProjectionLoc,
                                 CameraSetup->Projection.e);
                UniformBool(GL->SimpleShader.HasSkinningLoc,
                            Mesh->TypeCtx.MeshType == Mesh_Skinned);
                UniformInt(GL->SimpleShader.BonesCountLoc, 
                           entry->BoneCount);
                
                if(entry->BoneCount){
                    glUniformMatrix4fv(GL->SimpleShader.BoneTransformsLoc,
                                       entry->BoneCount,
                                       GL_TRUE,
                                       (const GLfloat*)entry->BoneTransforms[0].e);
                }
                
                // NOTE(Dima): Setting FS uniforms
                b32 AlbedoIsSet = 0;
                b32 NormalsIsSet = 0;
                b32 SpecularIsSet = 0;
                b32 EmissiveIsSet = 0;
                
                if(entry->Material){
                    material_info* Mat = entry->Material;
                    
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
                
                u32 TexturesSetFlags = 
                    (AlbedoIsSet) |
                    (NormalsIsSet << 1) |
                    (SpecularIsSet << 2) |
                    (EmissiveIsSet << 3);
                
                UniformVec3(GL->SimpleShader.AlbedoColorLoc, entry->AlbedoColor);
                UniformInt(GL->SimpleShader.TexturesSetFlagsLoc, TexturesSetFlags);
                
                glBindVertexArray(Mesh->Handles.Handles[0]);
                glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
                
                glDisable(GL_DEPTH_TEST);
                //GlFreeMeshHandles(&Mesh->Handles);
            }break;
        }
        
        at += Header->dataSize;
    }
}

INTERNAL_FUNCTION void GlOutputPass(gl_state* GL, render_pass* Pass){
    for(int i = 0; i < Pass->StacksCount; i++){
        GlOutputStack(GL, Pass->Stacks[i], &Pass->CameraSetup);
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

INTERNAL_FUNCTION void GlFinalOutput(gl_state* GL, render_state* Render){
    // NOTE(Dima): outputing lines
    OutputRenderLines(GL, Render, Render->Passes[0].CameraSetup.ViewProjection);
    
    // NOTE(Dima): Outputing gui geometry
    glEnable(GL_SCISSOR_TEST);
    OutputGuiGeometry(GL, Render);
    OutputGuiGeometryLines(GL, Render);
    glDisable(GL_SCISSOR_TEST);
}

INTERNAL_FUNCTION void GlOutputRender(gl_state* GL, render_state* Render){
    FUNCTION_TIMING();
    
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // NOTE(Dima): Calculating gui orthographic projection matrix
    int WindowWidth = 1366;
    int WindowHeight = 768;
    int InitWindowWidth = WindowWidth;
    int InitWindowHeight = WindowHeight;
    
    if(Render->FrameInfoIsSet){
        WindowWidth = Render->FrameInfo.Width;
        WindowHeight = Render->FrameInfo.Height;
        InitWindowWidth = Render->FrameInfo.InitWidth;
        InitWindowHeight = Render->FrameInfo.InitHeight;
    }
    
    float a = 2.0f / (float)InitWindowWidth;
    float b = 2.0f / (float)InitWindowHeight;
    
    GLfloat GuiOrtho[16] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, -b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    GL->GuiOrtho = Floats2Matrix(GuiOrtho);
    
    glViewport(0.0f, 0.0f, WindowWidth, WindowHeight);
    
    // NOTE(Dima): Actual rendering
    if(Render->API.RendererType == Renderer_Software){
        if(Render->FrameInfoIsSet){
            ASSERT(Render->FrameInfoIsSet);
            
            bmp_info* SoftBuf = Render->FrameInfo.SoftwareBuffer;
            if(SoftBuf){
                GlShowDynamicBitmap(GL, SoftBuf);
            }
            else{
                // NOTE(Dima): Render through WinAPI
            }
        }
    }
    else if (Render->API.RendererType == Renderer_OpenGL){
        for(int i = 0; i < Render->PassCount; i++){
            GlOutputPass(GL, &Render->Passes[i]);
        }
        
        GlFinalOutput(GL, Render);
    }
}
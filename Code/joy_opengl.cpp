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
    
	Platform_Read_File_Result vFile = PlatformReadFile(vertexPath);
	Platform_Read_File_Result fFile = PlatformReadFile(fragmentPath);
    Platform_Read_File_Result gFile = {};
    char* toPassGeometryData = 0;
    if(geometryPath){
        gFile = PlatformReadFile(geometryPath);
        toPassGeometryData = (char*)gFile.data;
    }
    
	result->ID = GlLoadFromSource(
		(char*)vFile.data, 
		(char*)fFile.data, 
        toPassGeometryData);
    
    ResultShader.ID = result->ID;
    ResultShader.Type = GlShader_Simple;
    ResultShader._InternalProgramIndex = resultIndex;
    
	PlatformFreeFileMemory(&vFile);
	PlatformFreeFileMemory(&fFile);
    if(geometryPath){
        PlatformFreeFileMemory(&gFile);
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

void UniformV3(GLint Loc, float x, float y, float z){
    glUniform3f(Loc, x, y, z);
}

void UniformV3(GLint Loc, v3 A){
    glUniform3f(Loc, A.x, A.y, A.z);
}

void UniformV4(GLint Loc, float x, float y, float z, float w){
    glUniform4f(Loc, x, y, z, w);
}

void UniformV4(GLint Loc, v4 A){
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
    
    GLGETU(Albedo);
    GLGETU(Normals);
    GLGETU(AlbedoIsSet);
    GLGETU(NormalsIsSet);
    
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

INTERNAL_FUNCTION LOAD_SHADER_FUNC(LoadDebugGeomShader)
{
    gl_debuggeom_shader& Result = GL->DebugGeomShader;
    Result.Shader = GlLoadProgram(GL, PathV, PathF);
    
    GLGETU(ViewProjection);
    
    GLGETA(P);
    GLGETA(Color);
}

INTERNAL_FUNCTION inline void BindBufferAndFill(GLenum Target, GLenum Usage,
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

INTERNAL_FUNCTION GLuint GlAllocateTexture(bmp_info* bmp){
    GLuint GenerateTex;
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
        
        if(ArrayIsValid(GL->SimpleShader.PAttrLoc)){
            glEnableVertexAttribArray(GL->SimpleShader.PAttrLoc);
            glVertexAttribPointer(GL->SimpleShader.PAttrLoc,
                                  3, GL_FLOAT, GL_FALSE,
                                  Stride, (void*)Mesh->TypeCtx.OffsetP);
        }
        
        if(ArrayIsValid(GL->SimpleShader.UVAttrLoc)){
            glEnableVertexAttribArray(GL->SimpleShader.UVAttrLoc);
            glVertexAttribPointer(GL->SimpleShader.UVAttrLoc,
                                  2, GL_FLOAT, GL_FALSE,
                                  Stride, (void*)Mesh->TypeCtx.OffsetUV);
        }
        
        if(ArrayIsValid(GL->SimpleShader.NAttrLoc)){
            glEnableVertexAttribArray(GL->SimpleShader.NAttrLoc);
            glVertexAttribPointer(GL->SimpleShader.NAttrLoc,
                                  3, GL_FLOAT, GL_FALSE,
                                  Stride, (void*)Mesh->TypeCtx.OffsetN);
        }
        
        if(ArrayIsValid(GL->SimpleShader.TAttrLoc)){
            glEnableVertexAttribArray(GL->SimpleShader.TAttrLoc);
            glVertexAttribPointer(GL->SimpleShader.TAttrLoc,
                                  3, GL_FLOAT, GL_FALSE,
                                  Stride, (void*)Mesh->TypeCtx.OffsetT);
        }
        
        if(Mesh->TypeCtx.MeshType == Mesh_Skinned){
            if(ArrayIsValid(GL->SimpleShader.WeightsAttrLoc)){
                glEnableVertexAttribArray(GL->SimpleShader.WeightsAttrLoc);
                glVertexAttribPointer(GL->SimpleShader.WeightsAttrLoc,
                                      4, GL_FLOAT, GL_FALSE,
                                      Stride, (void*)Mesh->TypeCtx.OffsetWeights);
            }
            
            
            if(ArrayIsValid(GL->SimpleShader.BoneIDsAttrLoc)){
                glEnableVertexAttribArray(GL->SimpleShader.BoneIDsAttrLoc);
                glVertexAttribIPointer(GL->SimpleShader.BoneIDsAttrLoc,
                                       1, GL_UNSIGNED_INT,
                                       Stride, (void*)Mesh->TypeCtx.OffsetBoneIDs);
            }
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

void GlInit(gl_state* GL, assets* Assets){
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
    LoadDebugGeomShader(GL,
                        "../Data/Shaders/debug_geometry.vs",
                        "../Data/Shaders/debug_geometry.fs");
    
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
    
    // NOTE(Dima): Init Debug geometry buffer objects
    glGenVertexArrays(1, &GL->DEBUGGeomVAO);
    glGenBuffers(1, &GL->DEBUGGeomVBO);
    
    glBindVertexArray(GL->DEBUGGeomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->DEBUGGeomVBO);
    if(ArrayIsValid(GL->DebugGeomShader.PAttrLoc)){
        glEnableVertexAttribArray(GL->DebugGeomShader.PAttrLoc);
        glVertexAttribPointer(GL->DebugGeomShader.PAttrLoc,
                              3, GL_FLOAT, GL_FALSE,
                              6 * FS, 0);
    }
    if(ArrayIsValid(GL->DebugGeomShader.ColorAttrLoc)){
        glEnableVertexAttribArray(GL->DebugGeomShader.ColorAttrLoc);
        glVertexAttribPointer(GL->DebugGeomShader.ColorAttrLoc,
                              3, GL_FLOAT, GL_FALSE,
                              6 * FS, (void*)(3 * FS));
    }
    glBindVertexArray(0);
    
    // NOTE(Dima): Init GuiGeom buffer objects
    glGenVertexArrays(1, &GL->GuiGeomVAO);
    glGenBuffers(1, &GL->GuiGeomVBO);
    glGenBuffers(1, &GL->GuiGeomEBO);
    glGenBuffers(1, &GL->GuiGeomTB);
    
    glBindVertexArray(GL->GuiGeomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiGeomVBO);
    if(ArrayIsValid(GL->GuiGeomShader.PUVAttrLoc)){
        glEnableVertexAttribArray(GL->GuiGeomShader.PUVAttrLoc);
        glVertexAttribPointer(GL->GuiGeomShader.PUVAttrLoc, 
                              4, GL_FLOAT, GL_FALSE, 
                              8 * FS, 0);
    }
    
    if(ArrayIsValid(GL->GuiGeomShader.CAttrLoc)){
        glEnableVertexAttribArray(GL->GuiGeomShader.CAttrLoc);
        glVertexAttribPointer(GL->GuiGeomShader.CAttrLoc, 
                              4, GL_FLOAT, GL_FALSE, 
                              8 * FS, (void*)(4 * FS));
    }
    glBindVertexArray(0);
}

void GlFree(gl_state* GL){
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

void GlOutputStack(gl_state* GL, render_pass* Pass, render_stack* Stack){
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
            
            case RenderEntry_GuiGeom:{
                
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
                                 Pass->View.e);
                UniformMatrix4x4(GL->SimpleShader.ProjectionLoc,
                                 Pass->Projection.e);
                UniformBool(GL->SimpleShader.HasSkinningLoc,
                            Mesh->TypeCtx.MeshType == Mesh_Skinned);
                UniformInt(GL->SimpleShader.BonesCountLoc, 
                           entry->BoneCount);
                if(entry->BoneCount){
#if 0
                    glUniformMatrix4fv(GL->SimpleShader.BoneTransformsLoc,
                                       entry->BoneCount,
                                       GL_TRUE,
                                       (const GLfloat*)entry->BoneTransforms[0].e);
#else
                    for(int i = 0; i < entry->BoneCount; i++){
                        char Buf[256];
                        sprintf(Buf, "BoneTransforms[%d]", i);
                        
                        GLint MatrixLocation = glGetUniformLocation(GL->SimpleShader.Shader.ID, Buf);
                        glUniformMatrix4fv(MatrixLocation,
                                           1,
                                           GL_TRUE,
                                           (const GLfloat*)entry->BoneTransforms[i].e);
                    }
#endif
                }
                
                // NOTE(Dima): Setting FS uniforms
                b32 AlbedoIsSet = false;
                b32 NormalsIsSet = false;
                
                UniformBool(GL->SimpleShader.AlbedoIsSetLoc, AlbedoIsSet);
                UniformBool(GL->SimpleShader.NormalsIsSetLoc, NormalsIsSet);
                
#if 0
                glUniform1i(GL->SimpleShader.AlbedoLoc, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, ???);
                
                glUniform1i(GL->SimpleShader.NormalsLoc, 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, ???);
#endif
                
                glBindVertexArray(Mesh->Handles.Handles[0]);
                glDrawElements(GL_TRIANGLES, Mesh->IndicesCount, GL_UNSIGNED_INT, 0);
                
                glDisable(GL_DEPTH_TEST);
                //GlFreeMeshHandles(&Mesh->Handles);
            }break;
        }
        
        at += Header->dataSize;
    }
}

void GlOutputPass(gl_state* GL, render_pass* Pass){
    for(int i = 0; i < Pass->StacksCount; i++){
        GlOutputStack(GL, Pass, Pass->Stacks[i]);
    }
    
#if 1    
    // NOTE(Dima): Test lines
    v3 StartP = V3(0, 0, 0);
    v3 EndP = V3(10, 10, 10);
    v3 Color1 = V3(1.0f, 0.0f, 0.0f);
    v3 Color2 = V3(0.0f, 1.0f, 0.0f);
    v3 Color3 = V3(0.0f, 1.0f, 1.0f);
    
    static v3 Vertices[] = {
        StartP, Color1, 
        EndP, Color1,
        V3(0.0f), Color2,
        V3(-10.0f, 10.0f, 10.0f), Color2,
        V3(0.0f), Color3,
        V3(0.0f, 0.0f, 10.0f), Color3};
    
    glEnable(GL_DEPTH_TEST);
    
    glBindVertexArray(GL->DEBUGGeomVAO);
    BindBufferAndFill(GL_ARRAY_BUFFER,
                      GL_DYNAMIC_DRAW,
                      Vertices,
                      sizeof(Vertices),
                      GL->DEBUGGeomVBO);
    
    UseShader(&GL->DebugGeomShader.Shader);
    UniformMatrix4x4(GL->DebugGeomShader.ViewProjectionLoc,
                     Pass->ViewProjection.e);
    
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6);
    
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
#endif
}

void GlOutputRender(gl_state* GL, render_state* Render){
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // NOTE(Dima): Calculating gui orthographic projection matrix
    int WindowWidth = 1366;
    int WindowHeight = 768;
    if(Render->FrameInfoIsSet){
        WindowWidth = Render->FrameInfo.Width;
        WindowHeight = Render->FrameInfo.Height;
    }
    
    float a = 2.0f / WindowWidth;
    float b = 2.0f / WindowHeight;
    
    GLfloat GuiOrtho[16] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, -b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    GL->GuiOrtho = Floats2Matrix(GuiOrtho);
    
    // NOTE(Dima): Actual rendering
    if(Render->RendererType == Renderer_Software){
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
    else if (Render->RendererType == Renderer_OpenGL){
        for(int i = 0; i < Render->PassCount; i++){
            GlOutputPass(GL, &Render->Passes[i]);
        }
        
        // NOTE(Dima): Outputing gui
        glEnable(GL_SCISSOR_TEST);
        
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
        GLuint TexBufTex;
        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &TexBufTex);
        glBindTexture(GL_TEXTURE_BUFFER, TexBufTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R8I, GL->GuiGeomTB);
        
        UseShader(&GL->GuiGeomShader.Shader);
        UniformMatrix4x4(GL->GuiGeomShader.ProjectionLoc, GL->GuiOrtho.e);
        
        // NOTE(Dima): Passing Bitmap uniforms to shader
        glUniform1i(GL->GuiGeomShader.BitmapLoc, 0);
        glUniform1i(GL->GuiGeomShader.TriangleGeomTypesLoc, 1);
        
        //glBindVertexArray(GL->GuiGeomVAO);
        
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
        
        glDisable(GL_SCISSOR_TEST);
    }
}
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
        platform.OutputString(InfoLog);
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
        platform.OutputString(InfoLog);
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
            platform.OutputString(InfoLog);
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
        platform.OutputString(InfoLog);
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
        platform.OutputString(InfoLog);
        assert(Success);
    }
    
    
	GLuint Program = glCreateProgram();
	glAttachShader(Program, ComputeShader);
    glLinkProgram(Program);
    
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(Program, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        platform.OutputString(InfoLog);
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


INTERNAL_FUNCTION void GlSetShader(gl_shader* shader, gl_shader src){
    shader->ID = src.ID;
    shader->Type = src.Type;
    shader->_InternalProgramIndex = src._InternalProgramIndex;
}

INTERNAL_FUNCTION Gl_Screen_Shader& GlLoadScreenShader(gl_state* GL, char* pathV, char* pathF){
    Gl_Screen_Shader& Result = GL->ScreenShader;
    
    GlSetShader(&Result, GlLoadProgram(GL, pathV, pathF));
    
    GLGETU(ScreenTexture);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_Simple_Shader& GlLoadSimpleShader(gl_state* GL, char* PathV, char* PathF){
    Gl_Simple_Shader& Result = GL->SimpleShader;
    
    GlSetShader(&Result, GlLoadProgram(GL, PathV, PathF));
    
    GLGETU(Model);
    GLGETU(View);
    GLGETU(Projection);
    
    GLGETA(P);
    GLGETA(UV);
    GLGETA(N);
    GLGETA(T);
    GLGETA(C);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_GuiRect_Shader& GlLoadGuiRectShader(gl_state* GL, char* PathV, char* PathF){
    Gl_GuiRect_Shader& Result = GL->GuiRectShader;
    
    GlSetShader(&Result, GlLoadProgram(GL, PathV, PathF));
    
    GLGETU(Projection);
    GLGETU(BitmapIsSet);
    GLGETU(Bitmap);
    
    GLGETA(PUV);
    GLGETA(C);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_GuiGeom_Shader&
GlLoadGuiGeomShader(gl_state* GL, char* PathV, char* PathF){
    Gl_GuiGeom_Shader& Result = GL->GuiGeomShader;
    
    GlSetShader(&Result, GlLoadProgram(GL, PathV, PathF));
    
    GLGETU(Projection);
    GLGETU(Bitmap);
    GLGETU(TriangleGeomTypes);
    
    GLGETA(PUV);
    GLGETA(C);
    
    return(Result);
}

INTERNAL_FUNCTION GLuint GlAllocateTexture(Bmp_Info* bmp){
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

void GlInit(gl_state* GL, assets* Assets){
    GL->ProgramsCount = 0;
    GlLoadScreenShader(GL, 
                       "../Data/Shaders/screen.vs",
                       "../Data/Shaders/screen.fs");
    GlLoadSimpleShader(GL,
                       "../Data/Shaders/simple.vs",
                       "../Data/Shaders/simple.fs");
    GlLoadGuiRectShader(GL,
                        "../Data/Shaders/gui_rect.vs",
                        "../Data/Shaders/gui_rect.fs");
    GlLoadGuiGeomShader(GL,
                        "../Data/Shaders/gui_geom.vs",
                        "../Data/Shaders/gui_geom.fs");
    
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
    
    if(GlArrayIsValid(GL->GuiRectShader.PUVAttrLoc)){
        glEnableVertexAttribArray(GL->GuiRectShader.PUVAttrLoc);
        glVertexAttribPointer(0, 4, GL_FLOAT, 0, 8 * FS, 0);
    }
    
    if(GlArrayIsValid(GL->GuiRectShader.CAttrLoc)){
        glEnableVertexAttribArray(GL->GuiRectShader.CAttrLoc);
        glVertexAttribPointer(1, 4, GL_FLOAT, 0, 8 * FS, (void*)(4 * FS));
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Init LargeAtlas bitmap
    GL->LargeAtlasHandle = GlAllocateTexture(&Assets->MainLargeAtlas.Bitmap);
    
    // NOTE(Dima): Binding gui shader texture to 0 unit
    glUseProgram(GL->GuiRectShader.ID);
    glUniform1i(GL->GuiRectShader.BitmapLoc, 0);
    glUseProgram(0);
    
    // NOTE(Dima): Init GuiGeom buffer objects
    glGenVertexArrays(1, &GL->GuiGeomVAO);
    glGenBuffers(1, &GL->GuiGeomVBO);
    glGenBuffers(1, &GL->GuiGeomEBO);
    glGenBuffers(1, &GL->GuiGeomTB);
    
    glBindVertexArray(GL->GuiGeomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GL->GuiGeomVBO);
    if(GlArrayIsValid(GL->GuiGeomShader.PUVAttrLoc)){
        glEnableVertexAttribArray(GL->GuiGeomShader.PUVAttrLoc);
        glVertexAttribPointer(GL->GuiGeomShader.PUVAttrLoc, 
                              4, GL_FLOAT, 0, 8 * FS, 0);
    }
    
    if(GlArrayIsValid(GL->GuiGeomShader.CAttrLoc)){
        glEnableVertexAttribArray(GL->GuiGeomShader.CAttrLoc);
        glVertexAttribPointer(GL->GuiGeomShader.CAttrLoc, 
                              4, GL_FLOAT, 0, 8 * FS, (void*)(4 * FS));
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
    glUseProgram(GL->GuiRectShader.ID);
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


INTERNAL_FUNCTION inline void GlBindBufferAndFill(GLenum Target, GLenum Usage,
                                                  void* Data, size_t DataSize,
                                                  GLuint BufferName)
{
    GLint CurrentBufSize;
    glGetBufferParameteriv(Target, 
                           GL_BUFFER_SIZE,
                           &CurrentBufSize);
    
    glBindBuffer(Target, BufferName);
    if(DataSize > CurrentBufSize){
        // NOTE(Dima): Reallocating or initializing at the first time
        glBufferData(Target, DataSize, Data, Usage);
    }
    else{
        glBufferSubData(Target, 0, DataSize, Data);
    }
}

INTERNAL_FUNCTION void GlShowDynamicBitmap(gl_state* GL, Bmp_Info* bmp){
    
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
    glUseProgram(GL->ScreenShader.ID);
    
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

void GlOutputCommands(gl_state* GL, render_stack* Stack){
    u8* at = (u8*)Stack->MemBlock.Base;
    u8* StackEnd = (u8*)Stack->MemBlock.Base + Stack->MemBlock.Used;
    
    while (at < StackEnd) {
        render_entry_header* Header = (render_entry_header*)at;
        
        at += sizeof(render_entry_header);
        
        switch(Header->type){
            case RenderEntry_ClearColor:{
                RENDER_GET_ENTRY(RenderEntryClearColor);
                
                glClearColor(entry->clearColor01.r,
                             entry->clearColor01.g,
                             entry->clearColor01.b,
                             1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(RenderEntryBitmap);
                
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
                
#if 1                
                GlRenderGuiRect(GL, GL->GuiOrtho.e, JOY_TRUE, 
                                entry->Bitmap->Handle,
                                RectArr, sizeof(RectArr));
#endif
                
            }break;
            
            case RenderEntry_GuiGeom:{
                
            }break;
            
            case RenderEntry_Mesh:{
                
            }break;
        }
        
        at += Header->dataSize;
    }
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
            
            Bmp_Info* SoftBuf = Render->FrameInfo.SoftwareBuffer;
            if(SoftBuf){
                GlShowDynamicBitmap(GL, SoftBuf);
            }
            else{
                // NOTE(Dima): Render through WinAPI
            }
        }
    }
    else if (Render->RendererType == Renderer_OpenGL){
        GlOutputCommands(GL, &Render->Stacks[0]);
        
        glBindVertexArray(GL->GuiGeomVAO);
        GlBindBufferAndFill(GL_ARRAY_BUFFER,
                            GL_DYNAMIC_DRAW,
                            Render->GuiGeom.Vertices,
                            Render->GuiGeom.VerticesCount * sizeof(render_gui_geom_vertex),
                            GL->GuiGeomVBO);
        
        GlBindBufferAndFill(GL_ELEMENT_ARRAY_BUFFER,
                            GL_DYNAMIC_DRAW,
                            Render->GuiGeom.Indices,
                            Render->GuiGeom.IndicesCount * sizeof(u32),
                            GL->GuiGeomEBO);
        
        GlBindBufferAndFill(GL_TEXTURE_BUFFER,
                            GL_DYNAMIC_DRAW,
                            Render->GuiGeom.TriangleGeomTypes,
                            Render->GuiGeom.TriangleGeomTypesCount,
                            GL->GuiGeomTB);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GL->LargeAtlasHandle);
        
        // NOTE(Dima): Passing geom types
        GLuint TexBufTex;
        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &TexBufTex);
        glBindTexture(GL_TEXTURE_BUFFER, TexBufTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R8I, GL->GuiGeomTB);
        
        GL->GuiGeomShader.Use();
        GL->GuiGeomShader.SetM44(GL->GuiGeomShader.ProjectionLoc, GL->GuiOrtho.e);
        
        // NOTE(Dima): Passing Bitmap uniforms to shader
        glUniform1i(GL->GuiGeomShader.BitmapLoc, 0);
        glUniform1i(GL->GuiGeomShader.TriangleGeomTypesLoc, 1);
        
        glBindVertexArray(GL->GuiGeomVAO);
        glDrawElements(GL_TRIANGLES, Render->GuiGeom.IndicesCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glDeleteTextures(1, &TexBufTex);
    }
}
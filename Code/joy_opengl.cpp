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

INTERNAL_FUNCTION Gl_Shader GlLoadProgram(Gl_State* gl, char* vertexPath, char* fragmentPath, char* geometryPath = 0) {
    assert(gl->ProgramsCount < ARRAY_COUNT(gl->Programs));
	int resultIndex = gl->ProgramsCount;
    Gl_Program* result = gl->Programs + gl->ProgramsCount++;
    
    Gl_Shader ResultShader = {};
    
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


INTERNAL_FUNCTION void GlSetShader(Gl_Shader* shader, Gl_Shader src){
    shader->ID = src.ID;
    shader->Type = src.Type;
    shader->_InternalProgramIndex = src._InternalProgramIndex;
}

INTERNAL_FUNCTION Gl_Screen_Shader& GlLoadScreenShader(Gl_State* gl, char* pathV, char* pathF){
    Gl_Screen_Shader& Result = gl->ScreenShader;
    
    GlSetShader(&Result, GlLoadProgram(gl, pathV, pathF));
    
    GLGETU(ScreenTexture);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_Simple_Shader& GlLoadSimpleShader(Gl_State* gl, char* PathV, char* PathF){
    Gl_Simple_Shader& Result = gl->SimpleShader;
    
    GlSetShader(&Result, GlLoadProgram(gl, PathV, PathF));
    
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

INTERNAL_FUNCTION Gl_GuiRect_Shader& GlLoadGuiRectShader(Gl_State* gl, char* PathV, char* PathF){
    Gl_GuiRect_Shader& Result = gl->GuiRectShader;
    
    GlSetShader(&Result, GlLoadProgram(gl, PathV, PathF));
    
    GLGETU(Projection);
    GLGETU(BitmapIsSet);
    GLGETU(Bitmap);
    
    GLGETA(PUV);
    GLGETA(C);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_GuiGeom_Shader&
GlLoadGuiGeomShader(Gl_State* gl, char* PathV, char* PathF){
    Gl_GuiGeom_Shader& Result = gl->GuiGeomShader;
    
    GlSetShader(&Result, GlLoadProgram(gl, PathV, PathF));
    
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

void GlInit(Gl_State* gl, Assets* assets){
    gl->ProgramsCount = 0;
    GlLoadScreenShader(gl, 
                       "../Data/Shaders/screen.vs",
                       "../Data/Shaders/screen.fs");
    GlLoadSimpleShader(gl,
                       "../Data/Shaders/simple.vs",
                       "../Data/Shaders/simple.fs");
    GlLoadGuiRectShader(gl,
                        "../Data/Shaders/gui_rect.vs",
                        "../Data/Shaders/gui_rect.fs");
    GlLoadGuiGeomShader(gl,
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
    
    glGenVertexArrays(1, &gl->ScreenVAO);
    glGenBuffers(1, &gl->ScreenVBO);
    
    glBindVertexArray(gl->ScreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl->ScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenFaceVerts), screenFaceVerts, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, 0, 4 * FS, 0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Initializing GuiRect mesh
    glGenVertexArrays(1, &gl->GuiRectVAO);
    glGenBuffers(1, &gl->GuiRectVBO);
    
    glBindVertexArray(gl->GuiRectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl->GuiRectVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 8 * FS, 0, GL_DYNAMIC_DRAW);
    
    if(GlArrayIsValid(gl->GuiRectShader.PUVAttrLoc)){
        glEnableVertexAttribArray(gl->GuiRectShader.PUVAttrLoc);
        glVertexAttribPointer(0, 4, GL_FLOAT, 0, 8 * FS, 0);
    }
    
    if(GlArrayIsValid(gl->GuiRectShader.CAttrLoc)){
        glEnableVertexAttribArray(gl->GuiRectShader.CAttrLoc);
        glVertexAttribPointer(1, 4, GL_FLOAT, 0, 8 * FS, (void*)(4 * FS));
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Init LargeAtlas bitmap
    gl->LargeAtlasHandle = GlAllocateTexture(&assets->MainLargeAtlas.Bitmap);
    
    // NOTE(Dima): Binding gui shader texture to 0 unit
    glUseProgram(gl->GuiRectShader.ID);
    glUniform1i(gl->GuiRectShader.BitmapLoc, 0);
    glUseProgram(0);
    
    // NOTE(Dima): Init GuiGeom buffer objects
    glGenVertexArrays(1, &gl->GuiGeomVAO);
    glGenBuffers(1, &gl->GuiGeomVBO);
    glGenBuffers(1, &gl->GuiGeomEBO);
    glGenBuffers(1, &gl->GuiGeomTB);
    
    glBindVertexArray(gl->GuiGeomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl->GuiGeomVBO);
    if(GlArrayIsValid(gl->GuiGeomShader.PUVAttrLoc)){
        glEnableVertexAttribArray(gl->GuiGeomShader.PUVAttrLoc);
        glVertexAttribPointer(gl->GuiGeomShader.PUVAttrLoc, 
                              4, GL_FLOAT, 0, 8 * FS, 0);
    }
    
    if(GlArrayIsValid(gl->GuiGeomShader.CAttrLoc)){
        glEnableVertexAttribArray(gl->GuiGeomShader.CAttrLoc);
        glVertexAttribPointer(gl->GuiGeomShader.CAttrLoc, 
                              4, GL_FLOAT, 0, 8 * FS, (void*)(4 * FS));
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GlFree(Gl_State* gl){
    // NOTE(Dima): Freeing all of the shader programs
    for(int ProgramIndex = 0;
        ProgramIndex < gl->ProgramsCount;
        ProgramIndex++)
    {
        glDeleteProgram(gl->Programs[ProgramIndex].ID);
    }
    
    // NOTE(Dima): Deleting main large atlas texture
    glDeleteTextures(1, &gl->LargeAtlasHandle);
    
    // NOTE(Dima): Free GuiGeom buffer objects
    glDeleteBuffers(1, &gl->GuiGeomEBO);
    glDeleteBuffers(1, &gl->GuiGeomVBO);
    glDeleteBuffers(1, &gl->GuiGeomTB);
    glDeleteVertexArrays(1, &gl->GuiGeomVAO);
}

INTERNAL_FUNCTION void GlRenderGuiRect(Gl_State* gl,
                                       float* GuiOrtho,
                                       b32 BitmapSetFlag,
                                       GLuint BmpHandle,
                                       float* RectArr,
                                       size_t RectArrSize)
{
    glUseProgram(gl->GuiRectShader.ID);
    glUniform1i(gl->GuiRectShader.BitmapIsSetLoc, BitmapSetFlag);
    
    if(BitmapSetFlag == 1){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BmpHandle);
        glUniform1i(gl->GuiRectShader.BitmapLoc, 0);
    }
    
    glUniformMatrix4fv(gl->GuiRectShader.ProjectionLoc,
                       1, GL_TRUE,
                       GuiOrtho);
    
    glBindVertexArray(gl->GuiRectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl->GuiRectVBO);
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

INTERNAL_FUNCTION void GlShowDynamicBitmap(Gl_State* gl, Bmp_Info* bmp){
    
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
    glBindVertexArray(gl->ScreenVAO);
    glUseProgram(gl->ScreenShader.ID);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlitTex);
    glUniform1i(gl->ScreenShader.ScreenTextureLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glUseProgram(0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Freeing blit texture
    glDeleteTextures(1, &BlitTex);
}

void GlOutputCommands(Gl_State* gl, Render_Stack* stack){
    u8* at = (u8*)stack->MemBlock.Base;
    u8* stackEnd = (u8*)stack->MemBlock.Base + stack->MemBlock.Used;
    
    while (at < stackEnd) {
        Render_Entry_Header* header = (Render_Entry_Header*)at;
        
        at += sizeof(Render_Entry_Header);
        
        switch(header->type){
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
                GlRenderGuiRect(gl, gl->GuiOrtho.e, JOY_TRUE, 
                                entry->Bitmap->Handle,
                                RectArr, sizeof(RectArr));
#endif
                
            }break;
            
            case RenderEntry_GuiGeom:{
                
            }break;
            
            case RenderEntry_Mesh:{
                
            }break;
        }
        
        at += header->dataSize;
    }
}

void GlOutputRender(Gl_State* gl, Render_State* render){
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // NOTE(Dima): Calculating gui orthographic projection matrix
    int WindowWidth = 1366;
    int WindowHeight = 768;
    if(render->FrameInfoIsSet){
        WindowWidth = render->FrameInfo.Width;
        WindowHeight = render->FrameInfo.Height;
    }
    
    float a = 2.0f / WindowWidth;
    float b = 2.0f / WindowHeight;
    
    GLfloat GuiOrtho[16] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, -b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    gl->GuiOrtho = Floats2Matrix(GuiOrtho);
    
    // NOTE(Dima): Actual rendering
    if(render->RendererType == Renderer_Software){
        if(render->FrameInfoIsSet){
            ASSERT(render->FrameInfoIsSet);
            
            Bmp_Info* SoftBuf = render->FrameInfo.SoftwareBuffer;
            if(SoftBuf){
                GlShowDynamicBitmap(gl, SoftBuf);
            }
            else{
                // NOTE(Dima): Render through WinAPI
            }
        }
    }
    else if (render->RendererType == Renderer_OpenGL){
        GlOutputCommands(gl, &render->Stacks[0]);
        
        glBindVertexArray(gl->GuiGeomVAO);
        GlBindBufferAndFill(GL_ARRAY_BUFFER,
                            GL_DYNAMIC_DRAW,
                            render->GuiGeom.Vertices,
                            render->GuiGeom.VerticesCount * sizeof(Render_Gui_Geom_Vertex),
                            gl->GuiGeomVBO);
        
        GlBindBufferAndFill(GL_ELEMENT_ARRAY_BUFFER,
                            GL_DYNAMIC_DRAW,
                            render->GuiGeom.Indices,
                            render->GuiGeom.IndicesCount * sizeof(u32),
                            gl->GuiGeomEBO);
        
        GlBindBufferAndFill(GL_TEXTURE_BUFFER,
                            GL_DYNAMIC_DRAW,
                            render->GuiGeom.TriangleGeomTypes,
                            render->GuiGeom.TriangleGeomTypesCount,
                            gl->GuiGeomTB);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gl->LargeAtlasHandle);
        
        // NOTE(Dima): Passing geom types
        GLuint TexBufTex;
        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &TexBufTex);
        glBindTexture(GL_TEXTURE_BUFFER, TexBufTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R8I, gl->GuiGeomTB);
        
        gl->GuiGeomShader.Use();
        gl->GuiGeomShader.SetM44(gl->GuiGeomShader.ProjectionLoc, gl->GuiOrtho.e);
        
        // NOTE(Dima): Passing Bitmap uniforms to shader
        glUniform1i(gl->GuiGeomShader.BitmapLoc, 0);
        glUniform1i(gl->GuiGeomShader.TriangleGeomTypesLoc, 1);
        
        glBindVertexArray(gl->GuiGeomVAO);
        glDrawElements(GL_TRIANGLES, render->GuiGeom.IndicesCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glDeleteTextures(1, &TexBufTex);
    }
}
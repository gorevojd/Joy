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

INTERNAL_FUNCTION int GlLoadProgram(Gl_State* gl, char* vertexPath, char* fragmentPath, char* geometryPath = 0) {
    assert(gl->ProgramsCount < ARRAY_COUNT(gl->Programs));
	int resultIndex = gl->ProgramsCount;
    Gl_Program* result = gl->Programs + gl->ProgramsCount++;
    
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
    
	PlatformFreeFileMemory(&vFile);
	PlatformFreeFileMemory(&fFile);
    if(geometryPath){
        PlatformFreeFileMemory(&gFile);
    }
    
	return(resultIndex);
}

INTERNAL_FUNCTION Gl_Screen_Shader& GlLoadScreenShader(Gl_State* gl, char* pathV, char* pathF){
    Gl_Screen_Shader& Result = gl->ScreenShader;
    
    Result.ProgramIndex = GlLoadProgram(gl, pathV, pathF);
    
    Result.ScreenTextureLoc = GLGETU("ScreenTexture");
    GLGETUNAME(ScreenTexture);
    
    
    return(Result);
}

INTERNAL_FUNCTION Gl_Simple_Shader& GlLoadSimpleShader(Gl_State* gl, char* PathV, char* PathF){
    Gl_Simple_Shader& Result = gl->SimpleShader;
    
    Result.ProgramIndex = GlLoadProgram(gl, PathV, PathF);
    
    GLGETUNAME(Model);
    GLGETUNAME(View);
    GLGETUNAME(Projection);
    
    GLGETANAME(P);
    GLGETANAME(UV);
    GLGETANAME(N);
    GLGETANAME(T);
    GLGETANAME(C);
    
    return(Result);
}

INTERNAL_FUNCTION Gl_GuiRect_Shader& GlLoadGuiRectShader(Gl_State* gl, char* PathV, char* PathF){
    Gl_GuiRect_Shader& Result = gl->GuiRectShader;
    
    Result.ProgramIndex = GlLoadProgram(gl, PathV, PathF);
    
    GLGETUNAME(Projection);
    GLGETUNAME(BitmapIsSet);
    GLGETUNAME(Bitmap);
    
    GLGETANAME(PUV);
    GLGETANAME(C);
    
    return(Result);
}

void GlInit(Gl_State* gl){
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
}

void GlFree(Gl_State* gl){
    // NOTE(Dima): Freeing all of the shader programs
    for(int programIndex = 0;
        programIndex < gl->ProgramsCount;
        programIndex++)
    {
        glDeleteProgram(GLGETP(programIndex).ID);
    }
}

INTERNAL_FUNCTION void GlRenderGuiRect(Gl_State* gl,
                                       float* GuiOrtho,
                                       b32 BitmapIsSet,
                                       GLuint BmpHandle,
                                       float* RectArr,
                                       size_t RectArrSize)
{
    
    glUseProgram(GLGETPID(gl->GuiRectShader));
    glUniform1i(gl->GuiRectShader.BitmapIsSetLoc, BitmapIsSet);
    if(BitmapIsSet){
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
    
    if(BitmapIsSet){
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    glUseProgram(0);
}

void GlOutputCommands(Gl_State* gl, Render_Stack* stack){
    u8* at = (u8*)stack->mem;
	u8* stackEnd = (u8*)stack->mem + stack->memUsed;
    
    float a = 2.0f / stack->Width;
    float b = 2.0f / stack->Height;
    
    GLfloat GuiOrtho[16] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, -b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
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
            
            case RenderEntry_Gradient:{
                RENDER_GET_ENTRY(RenderEntryGradient);
                
                v2 Min = entry->rc.min;
                v2 Max = entry->rc.max;
                
                float r1 = entry->color1.r;
                float g1 = entry->color1.g;
                float b1 = entry->color1.b;
                
                float r2 = entry->color2.r;
                float g2 = entry->color2.g;
                float b2 = entry->color2.b;
                
                float RectArr1[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r1, g1, b1, 1.0f,
                    Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f,
                    Max.x, Min.y, 1.0f, 0.0f, r2, g2, b2, 1.0f,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r1, g1, b1, 1.0f,
                    Max.x, Min.y, 1.0f, 0.0f, r2, g2, b2, 1.0f,
                    Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f,
                };
                
                float RectArr2[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r2, g2, b2, 1.0f,
                    Max.x, Max.y, 1.0f, 1.0f, r2, g2, b2, 1.0f,
                    Max.x, Min.y, 1.0f, 0.0f, r1, g1, b1, 1.0f,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r2, g2, b2, 1.0f,
                    Max.x, Min.y, 1.0f, 0.0f, r1, g1, b1, 1.0f,
                    Min.x, Min.y, 0.0f, 0.0f, r1, g1, b1, 1.0f,
                };
                
                float *ArrToUse = 0;
                size_t ArrToUseSize = 0;
                if(entry->gradType == RenderEntryGradient_Horizontal){
                    ArrToUse = RectArr1;
                    ArrToUseSize = sizeof(RectArr1);
                }
                else if(entry->gradType == RenderEntryGradient_Vertical){
                    ArrToUse = RectArr2;
                    ArrToUseSize = sizeof(RectArr2);
                }
                else{
                    INVALID_CODE_PATH;
                }
                
                GlRenderGuiRect(gl, GuiOrtho, JOY_FALSE, 0,
                                ArrToUse, ArrToUseSize);
            }break;
            
            case RenderEntry_Rect:{
                RENDER_GET_ENTRY(RenderEntryRect);
                
                v2 Min = entry->p;
                v2 Max = Min + entry->dim;
                
                float r = entry->modulationColor01.r;
                float g = entry->modulationColor01.g;
                float b = entry->modulationColor01.b;
                float a = entry->modulationColor01.a;
                
                float RectArr[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Max.y, 1.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    Min.x, Min.y, 0.0f, 0.0f, r, g, b, a,
                };
                
                GlRenderGuiRect(gl, GuiOrtho, JOY_FALSE, 0,
                                RectArr, sizeof(RectArr));
            }break;
            
            case RenderEntry_Bitmap:{
                RENDER_GET_ENTRY(RenderEntryBitmap);
                
                if(!entry->bitmap->Handle){
                    GLuint GenerateTex;
                    glGenTextures(1, &GenerateTex);
                    glBindTexture(GL_TEXTURE_2D, GenerateTex);
                    
                    Bmp_Info* Bmp = entry->bitmap;
                    
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        Bmp->Width,
                        Bmp->Height,
                        0,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        Bmp->Pixels);
                    
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    
                    entry->bitmap->Handle = GenerateTex;
                }
                
                v2 Min = entry->topLeftP;
                v2 Max = V2(Min.x + entry->bitmap->WidthOverHeight * entry->pixelHeight,
                            Min.y + entry->pixelHeight);
                
                float r = entry->modulationColor01.r;
                float g = entry->modulationColor01.g;
                float b = entry->modulationColor01.b;
                float a = entry->modulationColor01.a;
                
                float RectArr[] = {
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Max.y, 1.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    
                    Min.x, Max.y, 0.0f, 1.0f, r, g, b, a,
                    Max.x, Min.y, 1.0f, 0.0f, r, g, b, a,
                    Min.x, Min.y, 0.0f, 0.0f, r, g, b, a,
                };
                
                GlRenderGuiRect(gl, GuiOrtho, JOY_TRUE, 
                                entry->bitmap->Handle,
                                RectArr, sizeof(RectArr));
            }break;
            
            case RenderEntry_Mesh:{
                
            }break;
            
            case RenderEntry_Glyph:{
                
            }break;
        }
        
        at += header->dataSize;
    }
}

void GlOutputRender(Gl_State* gl, Render_Stack* stack){
    glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
    
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    GlOutputCommands(gl, stack);
    
#if 0    
    // NOTE(Dima): Blit texture load
    GLuint BlitTex;
    glGenTextures(1, &BlitTex);
    glBindTexture(GL_TEXTURE_2D, BlitTex);
    glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		BlitBmp->Width,
		BlitBmp->Height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		BlitBmp->Pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // NOTE(Dima): Drawing screen rect
    glBindVertexArray(gl->ScreenVAO);
    glUseProgram(GLGETPID(gl->ScreenShader));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlitTex);
    glUniform1i(gl->ScreenShader.ScreenTextureLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glUseProgram(0);
    glBindVertexArray(0);
    
    // NOTE(Dima): Freeing blit texture
    glDeleteTextures(1, &BlitTex);
#endif
    
}
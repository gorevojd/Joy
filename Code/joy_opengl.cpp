#include "joy_opengl.h"

#include <assert.h>

INTERNAL_FUNCTION GLuint Gl_LoadFromSource(char* VertexSource, char* FragmentSource, char* GeometrySource = 0) {
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

INTERNAL_FUNCTION GLuint Gl_LoadFromSourceCompute(char* ComputeCode){
    
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

INTERNAL_FUNCTION int Gl_LoadProgram(Gl_State* gl, char* vertexPath, char* fragmentPath, char* geometryPath = 0) {
    assert(gl->programsCount < ARRAY_COUNT(gl->programs));
	int resultIndex = gl->programsCount;
    Gl_Program* result = gl->programs + gl->programsCount++;
    
	Platform_Read_File_Result vFile = PlatformReadFile(vertexPath);
	Platform_Read_File_Result fFile = PlatformReadFile(fragmentPath);
    Platform_Read_File_Result gFile = {};
    char* toPassGeometryData = 0;
    if(geometryPath){
        gFile = PlatformReadFile(geometryPath);
        toPassGeometryData = (char*)gFile.data;
    }
    
	result->id = Gl_LoadFromSource(
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

INTERNAL_FUNCTION Gl_Screen_Shader Gl_LoadScreenShader(Gl_State* gl, char* pathV, char* pathF){
    Gl_Screen_Shader result = {};
    
    result.programIndex = Gl_LoadProgram(gl, pathV, pathF);
    result.screenTextureLoc = GLGETU("ScreenTexture");
    
    return(result);
}

void GlInit(Gl_State* gl){
    gl->programsCount = 0;
    Gl_LoadScreenShader(gl, 
                        "../Data/Shaders/screen.vs",
                        "../Data/Shaders/screen.fs");
    
    size_t FS = sizeof(float);
    
    float screenFaceVerts[] = {
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };
    
    glGenVertexArrays(1, &gl->screenVAO);
    glGenBuffers(1, &gl->screenVBO);
    
    glBindVertexArray(gl->screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl->screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenFaceVerts), screenFaceVerts, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, 0, 4 * FS, 0);
    
    glBindVertexArray(0);
}

void GlFree(Gl_State* gl){
    // NOTE(Dima): Freeing all of the shader programs
    for(int programIndex = 0;
        programIndex < gl->programsCount;
        programIndex++)
    {
        glDeleteProgram(GLGETP(programIndex).id);
    }
}

void GlOutputRender(Gl_State* gl, Bmp_Info* blitBMP){
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // NOTE(Dima): Blit texture load
    GLuint blitTex;
    glGenTextures(1, &blitTex);
	glBindTexture(GL_TEXTURE_2D, blitTex);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		blitBMP->width,
		blitBMP->height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		blitBMP->pixels);
	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // NOTE(Dima): Drawing screen rect
    glBindVertexArray(gl->screenVAO);
    glUseProgram(GLGETPID(gl->screenShader));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blitTex);
    glUniform1i(gl->screenShader.screenTextureLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // NOTE(Dima): Freeing blit texture
    glDeleteTextures(1, &blitTex);
    
    glUseProgram(0);
    glBindVertexArray(0);
}
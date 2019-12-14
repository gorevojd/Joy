#include "joy_opengl.h"

#include <assert.h>

INTERNAL_FUNCTION GLuint GL_LoadFromSource(char* VertexSource, char* FragmentSource, char* GeometrySource = 0) {
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
        PlatformAPI.OutputString(InfoLog);
        assert(Success);
    }
    
	FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentSource, 0);
	glCompileShader(FragmentShader);
    
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(FragmentShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        PlatformAPI.OutputString(InfoLog);
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
            PlatformAPI.OutputString(InfoLog);
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
        PlatformAPI.OutputString(InfoLog);
        assert(Success);
	}
    
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
    if(GeometrySource){
        glDeleteShader(GeometryShader);
    }
    
	return(Program);
}

INTERNAL_FUNCTION GLuint GL_LoadFromSourceCompute(char* ComputeCode){
    
    char InfoLog[1024];
	int Success;
	
    
	GLuint ComputeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(ComputeShader, 1, &ComputeCode, 0);
	glCompileShader(ComputeShader);
	
	glGetShaderiv(ComputeShader, GL_COMPILE_STATUS, &Success);
	if (!Success) {
		glGetShaderInfoLog(ComputeShader, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        PlatformAPI.OutputString(InfoLog);
        assert(Success);
    }
    
    
	GLuint Program = glCreateProgram();
	glAttachShader(Program, ComputeShader);
    glLinkProgram(Program);
    
	glGetProgramiv(Program, GL_LINK_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(Program, sizeof(InfoLog), 0, InfoLog);
		//TODO(dima): Logging
        PlatformAPI.OutputString(InfoLog);
        assert(Success);
	}
    
	glDeleteShader(ComputeShader);
    
	return(Program);
}

INTERNAL_FUNCTION int GL_LoadProgram(gl_state* GL, char* VertexPath, char* FragmentPath, char* GeometryPath = 0) {
    assert(GL->ProgramsCount < ARRAY_COUNT(GL->Programs));
	int ResultIndex = GL->ProgramsCount;
    gl_program* Result = GL->Programs + GL->ProgramsCount++;
    
	platform_read_file_result VertexFile = PlatformReadFile(VertexPath);
	platform_read_file_result FragmentFile = PlatformReadFile(FragmentPath);
    platform_read_file_result GeometryFile = {};
    char* ToPassGeometryData = 0;
    if(GeometryPath){
        GeometryFile = PlatformReadFile(GeometryPath);
        ToPassGeometryData = (char*)GeometryFile.Data;
    }
    
	Result->ID = GL_LoadFromSource(
		(char*)VertexFile.Data, 
		(char*)FragmentFile.Data, 
        ToPassGeometryData);
    
	PlatformFreeFileMemory(&VertexFile);
	PlatformFreeFileMemory(&FragmentFile);
    if(GeometryPath){
        PlatformFreeFileMemory(&GeometryFile);
    }
    
	return(ResultIndex);
}

INTERNAL_FUNCTION gl_screen_shader GL_LoadScreenShader(gl_state* GL, char* PathV, char* PathF){
    gl_screen_shader Result = {};
    
    Result.ProgramIndex = GL_LoadProgram(GL, PathV, PathF);
    Result.ScreenTextureLoc = GLGETU("ScreenTexture");
    
    return(Result);
}

void GL_Init(gl_state* GL){
    GL->ProgramsCount = 0;
    GL_LoadScreenShader(GL, 
                        "../Data/Shaders/screen.vs",
                        "../Data/Shaders/screen.fs");
    
    size_t FS = sizeof(float);
    
    float ScreenFaceVerts[] = {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenFaceVerts), ScreenFaceVerts, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, 0, 4 * FS, 0);
    
    glBindVertexArray(0);
}

void GL_Free(gl_state* GL){
    // NOTE(Dima): Freeing all of the shader programs
    for(int ProgramIndex = 0;
        ProgramIndex < GL->ProgramsCount;
        ProgramIndex++)
    {
        glDeleteProgram(GLGETP(ProgramIndex).ID);
    }
}

void GL_OutputRender(gl_state* GL, bmp_info* BlitBMP){
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // NOTE(Dima): Blit texture load
    GLuint BlitBMPTex;
    glGenTextures(1, &BlitBMPTex);
	glBindTexture(GL_TEXTURE_2D, BlitBMPTex);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		BlitBMP->Width,
		BlitBMP->Height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		BlitBMP->Pixels);
	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // NOTE(Dima): Drawing screen rect
    glBindVertexArray(GL->ScreenVAO);
    glUseProgram(GLGETPID(GL->ScreenShader));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlitBMPTex);
    glUniform1i(GL->ScreenShader.ScreenTextureLoc, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // NOTE(Dima): Freeing blit texture
    glDeleteTextures(1, &BlitBMPTex);
    
    glUseProgram(0);
    glBindVertexArray(0);
}
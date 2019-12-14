#version 330 core

layout (location = 0) in vec4 inPUV;

out vec2 vsTexCoords;

void main(){
	gl_Position = vec4(inPUV.xy, 0.0f, 1.0f);
	vsTexCoords = inPUV.zw;
}
#version 330 core

layout (location = 0) in vec4 inPUV;

out vec2 VsUV;

uniform bool UVInvertY;

void main(){
	vec2 ResultUV = inPUV.zw;

	if(UVInvertY){
		ResultUV.y = 1.0f - ResultUV.y;
	}

	gl_Position = vec4(inPUV.xy, 0.0f, 1.0f);
	VsUV = ResultUV;
}
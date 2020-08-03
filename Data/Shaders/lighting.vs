#version 330 core

layout (location = 0) in vec4 inPUV;

uniform bool UVInvertY;
uniform float FOVRadians;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(inPUV.xy, 0.0f, 1.0f);
	TexCoord = inPUV.zw;

	if(UVInvertY){
		TexCoord.y = 1.0f - TexCoord.y;
	}
}
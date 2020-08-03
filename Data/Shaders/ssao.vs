#version 330 core

layout (location = 0) in vec4 inPUV;

uniform bool UVInvertY;
uniform vec2 WidthHeight;
uniform float FOVRadians;

out vec2 TexCoord;
out vec2 ViewSpaceZDevidedP;
flat out vec2 WH;

void main()
{
	float AspectRatio = WidthHeight.x / WidthHeight.y;

	float TanHalfFOVOver2 = tan(FOVRadians * 0.5f);
	gl_Position = vec4(inPUV.xy, 0.0f, 1.0f);
	TexCoord = inPUV.zw;

	if(UVInvertY){
		TexCoord.y = 1.0f - TexCoord.y;
	}

	ViewSpaceZDevidedP.x = inPUV.x * AspectRatio * TanHalfFOVOver2;
	ViewSpaceZDevidedP.y = inPUV.y * TanHalfFOVOver2;

	WH = WidthHeight;
}
#version 330 core

in vec2 VsUV;
out vec4 Color;

uniform sampler2D DepthTexture;

float Near = 0.2f;
float Far = 500.0f;

void main(){
	float d = texture2D(DepthTexture, VsUV).r;
	
	float z_ndc = d * 2.0f - 1.0f;

	float lin_depth = (2.0f * Near * Far) / (Far + Near - z_ndc * (Far - Near));

//	Color = vec4(vec3(lin_depth), 1.0f);
	Color = vec4(vec3(d), 1.0f);

	//Color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
}
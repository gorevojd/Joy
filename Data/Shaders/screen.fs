#version 330 core

in vec2 vsTexCoords;

uniform sampler2D ScreenTexture;

out vec4 Color;

void main(){
	Color = texture(ScreenTexture, vsTexCoords);
	//Color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
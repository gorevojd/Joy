#version 330 core

in vec2 VsUV;

uniform sampler2D ScreenTexture;

out vec4 Color;

void main(){
	Color = texture(ScreenTexture, VsUV);
}
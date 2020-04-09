#version 330 core

layout (location = 0) in vec3 P;
layout (location = 1) in vec3 Color;

uniform mat4 ViewProjection;

out vec3 VsColor;

void main(){
    VsColor = Color;
    gl_Position = vec4(P, 1.0f) * ViewProjection;
}
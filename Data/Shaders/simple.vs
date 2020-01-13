#version 330 core

layout (location = 0) in vec3 P;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 N;
layout (location = 3) in vec3 T;
layout (location = 4) in vec3 C;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out Vertex_Shader_Out{
    vec3 WorldP;
    vec3 WorldN;
    vec3 C;
    vec2 UV;
} VsOut;

void main(){
    vec4 WorldP = vec4(P, 1.0f) * Model;
    vec4 ViewP = WorldP * View;
    vec4 ProjectedP = ViewP * Projection;
    
    gl_Position = ProjectedP;
}
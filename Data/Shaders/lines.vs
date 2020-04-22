#version 330 core

layout (location = 0) in vec3 P;

uniform mat4 ViewProjection;
uniform samplerBuffer ColorsTexture;

out vec3 VsColor;

void main(){
    gl_Position = vec4(P, 1.0f) * ViewProjection;
    
    vec3 LineColor = texelFetch(ColorsTexture, gl_VertexID / 2).rgb;
    VsColor = LineColor;
}
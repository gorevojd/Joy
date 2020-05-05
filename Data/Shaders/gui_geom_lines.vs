#version 330 core

layout (location = 0) in vec2 P;

uniform mat4 Projection;
uniform samplerBuffer ColorsTexture;

out vec4 VsColor;

void main(){
    gl_Position = vec4(P.x, P.y, 0.0f, 1.0f) * Projection;
    
    VsColor = texelFetch(ColorsTexture, gl_VertexID / 2);
}
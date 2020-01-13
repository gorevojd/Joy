#version 330 core

in Vertex_Shader_Out{
    vec3 WorldP;
    vec3 WorldN;
    vec3 C;
    vec2 UV;
} FsIn;

out vec4 Color;

void main(){
    Color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
}

#version 330 core

layout (location = 0) in vec4 PUV;
layout (location = 1) in vec4 C;

uniform mat4 Projection;
uniform isamplerBuffer TriangleGeomTypes;

out Vertex_Shader_Out{
    vec2 UV;
    vec4 C;
    flat int TriangleGeomType;
} VsOut;


void main(){
    gl_Position = vec4(PUV.x, PUV.y, 0.0f, 1.0f) * Projection;
    
    VsOut.UV = PUV.zw;
    VsOut.C = C;
    VsOut.TriangleGeomType = texelFetch(TriangleGeomTypes, gl_VertexID / 2).r;
    //VsOut.TriangleGeomType = 1u;
}
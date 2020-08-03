#version 330 core

layout (location = 0) in vec3 P;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec3 N;
layout (location = 3) in vec3 T;
layout (location = 4) in vec4 Weights;
layout (location = 5) in uint BoneIDs;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform bool HasSkinning;
uniform int BonesCount;
uniform mat4 BoneTransforms[128];

out Vertex_Shader_Out{
    vec3 WorldP;
	vec3 ViewN;
    vec2 UV;
} VsOut;

void main(){
    vec3 ModelSpaceP = P;
    vec3 ModelSpaceN = N;
    
    // NOTE(Dima): Producing some skinning
    if(HasSkinning && (BonesCount > 0)){
        
        uint BoneIndex0 = BoneIDs & 255u;
        uint BoneIndex1 = (BoneIDs >> 8u) & 255u;
        uint BoneIndex2 = (BoneIDs >> 16u) & 255u;
        uint BoneIndex3 = (BoneIDs >> 24u) & 255u;
        
        mat4 Tran = BoneTransforms[BoneIndex0] * Weights.x;
        Tran += BoneTransforms[BoneIndex1] * Weights.y;
        Tran += BoneTransforms[BoneIndex2] * Weights.z;
        Tran += BoneTransforms[BoneIndex3] * Weights.w;
        
        vec4 SumP = vec4(ModelSpaceP, 1.0f) * Tran;
        vec4 SumN = vec4(ModelSpaceN, 0.0f) * Tran;
        
        ModelSpaceP = SumP.xyz;
        ModelSpaceN = SumN.xyz;
    }    

    // NOTE(Dima): Usual calculations
    vec4 WorldP = vec4(ModelSpaceP, 1.0f) * Model;
    vec4 ViewP = WorldP * View;
    vec4 ProjectedP = ViewP * Projection;
    
	//float CamToVertexLen = length(ViewP.xyz);
	float CamToVertexLen = ViewP.z;
	VsOut.WorldP = WorldP.xyz;
    VsOut.ViewN = (vec4(normalize(ModelSpaceN * transpose(inverse(mat3(Model)))), 0.0f) * View).xyz;
    VsOut.UV = vec2(UV.x, 1.0f - UV.y);
    
    gl_Position = ProjectedP;
}
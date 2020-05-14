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
    vec3 WorldN;
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
        
#if 0        
        mat4 Tran0 = BoneTransforms[0] * Weights.x;
        mat4 Tran1 = BoneTransforms[0] * Weights.y;
        mat4 Tran2 = BoneTransforms[0] * Weights.z;
        mat4 Tran3 = BoneTransforms[0] * Weights.w;
#endif
        
        mat4 Tran0 = BoneTransforms[BoneIndex0] * Weights.x;
        mat4 Tran1 = BoneTransforms[BoneIndex1] * Weights.y;
        mat4 Tran2 = BoneTransforms[BoneIndex2] * Weights.z;
        mat4 Tran3 = BoneTransforms[BoneIndex3] * Weights.w;
        
        vec4 TempP = vec4(ModelSpaceP, 1.0f);
        vec4 TempN = vec4(ModelSpaceN, 0.0f);
        
        vec4 SumP = TempP * Tran0;
        SumP += TempP * Tran1;
        SumP += TempP * Tran2;
        SumP += TempP * Tran3;
        
        vec4 SumN = TempN * Tran0;
        SumN += TempN * Tran1;
        SumN += TempN * Tran2;
        SumN += TempN * Tran3;
        
        ModelSpaceP = SumP.xyz;
        ModelSpaceN = SumN.xyz;
    }
    
    // NOTE(Dima): Usual calculations
    vec4 WorldP = vec4(ModelSpaceP, 1.0f) * Model;
    vec4 ViewP = WorldP * View;
    vec4 ProjectedP = ViewP * Projection;
    
    VsOut.WorldP = WorldP.xyz;
    VsOut.WorldN = normalize(ModelSpaceN * transpose(inverse(mat3(Model))));
    VsOut.UV = vec2(UV.x, 1.0f - UV.y);
    
    gl_Position = ProjectedP;
}
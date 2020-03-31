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
    if(HasSkinning){
        mat4 ResultTranMatrix = mat4(vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f));
        
        // NOTE(Dima): Iterating through weights and bones and apply to ResultTranMatrix
        for(uint WeightIndex = 0u;
            WeightIndex < 4u;
            WeightIndex++)
        {
            uint BoneID = BoneIDs >> (8u * WeightIndex) & 255u;
            
            float Weight = Weights[WeightIndex];
            
            ResultTranMatrix += BoneTransforms[BoneID] * Weight;
        }
        
        // NOTE(Dima): Transforming position and normals
        vec4 ResultTranP = vec4(ModelSpaceP, 1.0f) * ResultTranMatrix;
        vec4 ResultTranN = vec4(ModelSpaceN, 0.0f) * ResultTranMatrix;
        
        ModelSpaceP = ResultTranP.xyz;
        ModelSpaceN = ResultTranN.xyz;
    }
    
    // NOTE(Dima): Usual calculations
    vec4 WorldP = vec4(ModelSpaceP, 1.0f) * Model;
    vec4 ViewP = WorldP * View;
    vec4 ProjectedP = ViewP * Projection;
    
    VsOut.WorldP = WorldP.xyz;
    VsOut.WorldN = normalize(ModelSpaceN * transpose(inverse(mat3(Model))));
    VsOut.UV = UV;
    
    gl_Position = ProjectedP;
}
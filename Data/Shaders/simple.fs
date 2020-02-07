#version 330 core

in Vertex_Shader_Out{
    vec3 WorldP;
    vec3 WorldN;
    vec3 C;
    vec2 UV;
} FsIn;

out vec4 Color;

vec3 CalcDirLit(vec3 FragP, vec3 FragN, vec3 FragC){
    vec3 DirLitDir = vec3(0.5f, -0.5f, -0.5f);
    DirLitDir = normalize(DirLitDir);
    vec3 DirLitColor = vec3(1.0f, 1.0f, 1.0f);
    
    vec3 ToLit = normalize(-DirLitDir);
    
    float DiffuseFactor = clamp(dot(ToLit, FragN), 0.0f, 1.0f);
    vec3 Result = DiffuseFactor * DirLitColor * FragC;
    
    return(Result);
}

void main(){
    vec3 FragP = FsIn.WorldP;
    vec3 FragN = normalize(FsIn.WorldN);
    
    float AmbientFactor = 0.05f;
    vec3 Ambient = vec3(AmbientFactor);
    
    vec3 ResultColor = vec3(0.0f, 0.0f, 0.0f);
    ResultColor += Ambient + CalcDirLit(FragP, FragN, FsIn.C);
    
    Color = vec4(ResultColor, 1.0f);
    //Color = vec4(FsIn.C, 1.0f);
}

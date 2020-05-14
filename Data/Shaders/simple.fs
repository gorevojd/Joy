#version 330 core

in Vertex_Shader_Out{
    vec3 WorldP;
    vec3 WorldN;
    vec2 UV;
} FsIn;

out vec4 Color;

uniform vec3 AlbedoColor;

uniform sampler2D Albedo;
uniform sampler2D Normals;
uniform sampler2D Specular;
uniform sampler2D Emissive;

uniform int TexturesSetFlags;

const int ALBEDO_SET_FLAG = 1;
const int NORMALS_SET_FLAG = 1 << 1;
const int SPECULAR_SET_FLAG = 1 << 2;
const int EMISSIVE_SET_FLAG = 1 << 3;

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
    
    bool AlbedoIsSet = (TexturesSetFlags & ALBEDO_SET_FLAG) != 0;
    bool SpecularIsSet = (TexturesSetFlags & SPECULAR_SET_FLAG) != 0;
    bool NormalsIsSet = (TexturesSetFlags & NORMALS_SET_FLAG) != 0;
    bool EmissiveIsSet = (TexturesSetFlags & EMISSIVE_SET_FLAG) != 0;
    
    float AmbientFactor = 0.05f;
    vec3 Ambient = vec3(AmbientFactor);
    
    vec3 SampledAlbedo = AlbedoColor;
    if(AlbedoIsSet){
        SampledAlbedo = texture2D(Albedo, FsIn.UV).rgb;
    }
    
    vec3 ResultColor = vec3(0.0f, 0.0f, 0.0f);
    ResultColor += Ambient + CalcDirLit(FragP, FragN, SampledAlbedo);
    
    Color = vec4(ResultColor, 1.0f);
}

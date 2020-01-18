#version 330 core

uniform sampler2D Bitmap;

in Vertex_Shader_Out{
    vec2 UV;
    vec4 C;
    flat int TriangleGeomType;
} FsIn;

out vec4 Color;

void main(){
    vec4 SampledColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if(FsIn.TriangleGeomType == 1){
        SampledColor = texture2D(Bitmap, FsIn.UV);
    }
    
    Color = SampledColor * FsIn.C;
}
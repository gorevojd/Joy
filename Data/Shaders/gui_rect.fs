#version 330 core

uniform bool BitmapIsSet;
uniform sampler2D Bitmap;

in Vertex_Shader_Out{
    vec2 UV;
    vec4 C;
} FsIn;

out vec4 Color;

void main(){
    vec4 SampledColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if(BitmapIsSet){
        SampledColor = texture2D(Bitmap, FsIn.UV);
    }
    
    Color = SampledColor * FsIn.C;
}
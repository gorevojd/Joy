#version 330 core

in vec3 VsColor;

out vec4 OutColor;

void main(){
    OutColor = vec4(VsColor, 1.0f);
    //OutColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
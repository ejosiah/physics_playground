#version 460

layout(location=0) in struct {
    vec4 position;
    vec4 color;
} fs_in;

layout(location=0) out vec4 fragColor;

void main(){
    fragColor = fs_in.color;
}
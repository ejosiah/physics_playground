#version 460

layout(push_constant) uniform Constants {
    mat4 model;
    mat4 view;
    mat4 projection;
    float spacing;
    int tableSize;
};

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out struct {
    vec4 position;
    vec4 color;
} vs_out;

void main(){
    vs_out.position.xy = position;
    vs_out.color = color;

    gl_Position = projection * view * model * vec4(position, 0, 1);
}
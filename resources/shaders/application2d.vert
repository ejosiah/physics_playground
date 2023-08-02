#version 460

struct Instance{
    mat4 xform;
    vec4 color;
};

layout(location = 0) in vec3 position;

layout(set = 0, binding = 0) buffer InstanceData {
    Instance i[];
};

layout(set = 0, binding = 1) uniform CameraData {
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec4 color;
} vs_out;

void main(){
    vs_out.color = i[gl_InstanceIndex].color;
    gl_Position = projection * view * i[gl_InstanceIndex].xform * vec4(position, 1);
}
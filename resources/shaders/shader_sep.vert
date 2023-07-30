#version 460

layout(set = 0, binding = 0) buffer ParticlePosSSBO {
    vec2 particlePosition[];
};

layout(set = 0, binding = 1) buffer ParticleRadiusSSBO {
    float particleRadius[];
};

layout(set = 0, binding = 2) buffer ParticleColorSSBO {
    vec4 colors[];
};

layout(push_constant) uniform Constants {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout(location = 0) out struct {
    vec4 position;
    vec4 color;
} vs_out;

void main(){
    vec2 center = particlePosition[gl_InstanceIndex];
    float radius = particleRadius[gl_InstanceIndex];
    vec2 wPos = center + position * radius;
    vs_out.position = projection * view * model * vec4(wPos, 0, 1);
    vs_out.color = colors[gl_InstanceIndex];

    gl_Position = vs_out.position;
}
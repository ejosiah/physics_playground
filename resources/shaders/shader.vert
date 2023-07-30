#version 460

struct Particle {
    vec2 cPosition;
    vec2 pPosition;
    vec2 velocity;
    float inverseMass;
    float restitution;
    float radius;
    int padding0;
    int padding1;
    int padding2;
};

layout(set = 0, binding = 0) buffer ParticleSSBO {
    Particle particles[];
};

layout(set = 0, binding = 1) buffer ParticleColorSSBO {
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
    Particle p = particles[gl_InstanceIndex];
    vec2 wPos = p.cPosition + position * p.radius;
    vs_out.position = projection * view * model * vec4(wPos, 0, 1);
    vs_out.color = colors[gl_InstanceIndex];

    gl_Position = vs_out.position;
}
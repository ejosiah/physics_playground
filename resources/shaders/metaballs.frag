#version 460
#include "spacial_hash.glsl"

layout(location=0) in struct {
    vec4 position;
    vec4 color;
} fs_in;

layout(location=0) out vec4 fragColor;

void main(){
    float d = -1000000;
    vec2 p = fs_in.position.xy;
    int n = query(p, vec2(spacing));
    float r = spacing * 0.5;
    for(int i = 0; i < n; i++){
        vec2 c = particlePosition[cellEntries[queryId[i]]];
        d = max(d, distance(c, p));
    }

    fragColor = vec4(d, 0, 0, d);

}
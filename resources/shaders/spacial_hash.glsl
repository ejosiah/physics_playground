layout(set = 0, binding = 0) buffer ParticlePosSSBO {
    vec2 particlePosition[];
};


layout(set = 1, binding = 0) buffer SPACIAL_HASH_COUNTS {
    int counts[];
};

layout(set = 1, binding = 1) buffer SPACIAL_HASH_ENTRIES {
    int cellEntries[];
};

layout(push_constant) uniform Constants {
    mat4 model;
    mat4 view;
    mat4 projection;
    float spacing;
    int tableSize;
};

ivec2 intCoords(vec2 position)  {
    return ivec2(floor(position / spacing));
}

int hash(ivec2 pid) {
    int h = 541 * pid.x + 79 * pid.y;

    return abs(h) % tableSize;
}


int queryId[100];
int query(vec2 position, vec2 maxDist) {

    ivec2 d0 = intCoords(position - maxDist);
    ivec2 d1 = intCoords(position - maxDist);

    int querySize = 0;

    for (int xi = d0.x; xi <= d1.x; ++xi) {
        for (int yi = d0.y; yi <= d1.y; ++yi) {
            int h = hash(ivec2(xi, yi));
            int start = counts[h];
            int end = counts[h + 1];

            int size = end - start;

            for (int i = start; i < end; ++i) {
                queryId[querySize] = cellEntries[i];
                querySize++;
            }
        }
    }
    return querySize;
}


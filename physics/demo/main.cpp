#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>

#include "random_emitter.h"

int main(int, char**){
    Bounds2D bounds{ glm::vec2(0), glm::vec2(20) };
    auto emitter = std::make_unique<RandomParticleEmitter<SeparateFieldMemoryLayout>>();
    emitter->radius(0.5f);
    emitter->bounds(bounds);
    emitter->numParticles(50);
    emitter->restitution(0.5f);
    World2D<SeparateFieldMemoryLayout> world{"physics world", bounds, {1024, 1024}, std::move(emitter)};
    world.run();
}
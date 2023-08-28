#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>

#include "random_emitter.h"
#include "point_particle_emitter2d.h"

int main(int, char**){
    Bounds2D bounds{ glm::vec2(0), glm::vec2(20) };
//    auto emitter = std::make_unique<RandomParticleEmitter<SeparateFieldMemoryLayout>>();
//    emitter->radius(0.5f);
//    emitter->bounds(bounds);
//    emitter->numParticles(400);
//    emitter->restitution(0.5f);
    float radius = 0.1f;
    auto builder = PointParticleEmitter2D<SeparateFieldMemoryLayout>::builder();
    builder
        .withOrigin({radius, bounds.upper.y - radius})
        .withDirection({1, 0})
        .withSpeed(10)
        .withSpreadAngleInDegrees(0)
        .withMaxNumberOfNewParticlesPerSecond(10)
        .withMaxNumberOfParticles(std::numeric_limits<int>::max())
        .withRandomSeed((1 << 20))
        .withRadius(radius)
        .withMass(1)
        .withRestitution(0.5);

    World2D<SeparateFieldMemoryLayout> world{"physics world", bounds, {1024, 1024}, std::move(builder.makeUnique())};
    world.run();
}
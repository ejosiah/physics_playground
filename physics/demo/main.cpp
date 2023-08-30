#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "random_emitter.h"
#include "point_particle_emitter2d.h"

int main(int, char**){
    Bounds2D bounds{ glm::vec2(0), glm::vec2(20) };

    float radius = 0.1f;
    auto builder = PointParticleEmitter2D<SeparateFieldMemoryLayout>::builder();
    builder
        .withOrigin({radius * 2, bounds.upper.y - 2 * radius})
//        .withOrigin({bounds.upper.x * 0.5, bounds.upper.y - 2 * radius})
        .withDirection({1, 0})
//        .withDirection({0, -1})
        .withSpeed(10)
        .withSpreadAngleInDegrees(0)
        .withMaxNumberOfNewParticlesPerSecond(10)
        .withMaxNumberOfParticles(std::numeric_limits<int>::max())
        .withRandomSeed((1 << 20))
        .withRadius(radius)
        .withMass(1)
        .withRestitution(0.5);

    Emitters<SeparateFieldMemoryLayout> emitters{};
    emitters.push_back(std::move(builder.makeUnique()));


    World2D<SeparateFieldMemoryLayout> world{"physics world", bounds, {1024, 1024}
    , std::move(emitters) };

    try{
        world.run();
    }catch(std::runtime_error& error){
        spdlog::error("error: {}", error.what());
    }
}
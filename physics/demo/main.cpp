#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include "random_emitter.h"
#include "point_particle_emitter2d.h"
#include "volume_emitter_2d.h"
#include "grid_point_generator2d.h"
#include "triangle_point_generator2d.h"
#include <memory>

int main(int, char**){
    Bounds2D bounds{ glm::vec2(0), glm::vec2(20) };

    float radius = 0.1f;
    auto builder = PointParticleEmitter2D<SeparateFieldMemoryLayout>::builder();
    builder
        .withOrigin({bounds.upper.x - radius * 2, bounds.upper.y - 2 * radius})
        .withDirection({-1, 0})
//        .withOrigin({radius * 2, bounds.upper.y - 2 * radius})
//        .withOrigin({bounds.upper.x * 0.5, bounds.upper.y - 2 * radius})
//        .withDirection({1, 0})
//        .withDirection({0, -1})
        .withSpeed(10)
        .withSpreadAngleInDegrees(0)
        .withMaxNumberOfNewParticlesPerSecond(20)
        .withMaxNumberOfParticles(1500)
        .withRandomSeed((1 << 20))
        .withRadius(radius)
        .withMass(1)
        .withRestitution(0.5);

    std::function<float(const glm::vec2& point)> sdf = [](const glm::vec2& point) {
        auto d0 = point.y - 15.0f;
        auto d1 = glm::distance(point, glm::vec2(10, 18)) - 1.5f;
//        return glm::min(d0, d1);
        return d0;
    };

    std::unique_ptr<PointGenerator2D> pointGenerator = std::make_unique<TrianglePointGenerator>();
    std::unique_ptr<ParticleEmitter<SeparateFieldMemoryLayout>> vemitter =
            std::make_unique<VolumeEmitter2D<SeparateFieldMemoryLayout>>(std::move(sdf), std::move(pointGenerator), shrink(bounds, radius), radius * 2);

    Emitters<SeparateFieldMemoryLayout> emitters{};
//    emitters.push_back(std::move(builder.makeUnique()));
    emitters.push_back(std::move(vemitter));

//    builder
//        .withOrigin({bounds.upper.x - radius * 2, bounds.upper.y - 2 * radius})
//        .withDirection({-1, 0});
//
//    emitters.push_back(std::move(builder.makeUnique()));

//    for(int i = 1; i < 10; i++) {
//        builder
//                .withOrigin({bounds.upper.x - radius * 2, bounds.upper.y - 2 * (radius + radius * i)});
//        emitters.push_back(std::move(builder.makeUnique()));
//    }

    World2D<SeparateFieldMemoryLayout> world{"physics world",  bounds, {1024, 1024}
    , std::move(emitters), radius };

    try{
        world.run();
    }catch(std::runtime_error& error){
        spdlog::error("error: {}", error.what());
    }
}
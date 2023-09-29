#include "sph/sph_sim.h"
#include "volume_emitter_2d.h"
#include "point_generators.h"
#include "point_generators.h"
#include "point_particle_emitter2d.h"

int main(int, char**){
    Bounds2D bounds{ glm::vec2(0), glm::vec2(20) };
    float radius = 0.1f;


    std::function<float(const glm::vec2& point)> sdf = [](const glm::vec2& point) {
        auto d0 = point.y - 5.0f;
//        auto d1 = glm::distance(point, glm::vec2(10, 15)) - 1.0f;
//        return glm::min(d0, d1);
        return d0;
    };


    std::unique_ptr<PointGenerator2D> pointGenerator = std::make_unique<TrianglePointGenerator>();
    std::unique_ptr<ParticleEmitter<SeparateFieldMemoryLayout>> vemitter =
        std::make_unique<VolumeEmitter2D<SeparateFieldMemoryLayout>>(std::move(sdf), std::move(pointGenerator), shrink(bounds, radius), radius * 2);
    Emitters<SeparateFieldMemoryLayout> emitters{};
    emitters.push_back(std::move(vemitter));

//    auto builder = PointParticleEmitter2D<SeparateFieldMemoryLayout>::builder();
//    builder
//        .withDirection({-1, 0})
//        .withSpeed(10)
//        .withSpreadAngleInDegrees(0)
//        .withMaxNumberOfNewParticlesPerSecond(10)
//        .withMaxNumberOfParticles(100)
//        .withRandomSeed((1 << 20))
//        .withRadius(radius)
//        .withMass(1)
//        .withRestitution(0.5);
//
//    for(int i = 0; i < 10; i++) {
//        builder
//                .withOrigin({bounds.upper.x - radius * 2, bounds.upper.y - 2 * (radius + radius * i)});
//        emitters.push_back(std::move(builder.makeUnique()));
//    }

    SphSim sim("Smoothed particle hydrodynamics", bounds, {1024, 1024}, std::move(emitters));
    sim.run();

    return 0;
}
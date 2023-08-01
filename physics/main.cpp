#include <iostream>
#include "world2d.h"
#include <fmt/format.h>
#include "sdf2d.h"

constexpr float timeStep = 0.00416666666;
constexpr int NumParticles = 1000;
const Bounds2D Bounds = std::make_tuple(glm::vec2(0), glm::vec2(20));

auto rngg(float low,  float high, uint32_t seed) {
    return
            [engine=std::default_random_engine{seed}
                    , dist=std::uniform_real_distribution<float>{low, high}]() mutable {
                return dist(engine);
            };
}

std::vector<glm::vec2> position(NumParticles);
std::vector<glm::vec2> velocity(NumParticles);
std::vector<float> inverseMass(NumParticles, 1);
std::vector<float> restitution(NumParticles, 0.8);
std::vector<float> radius(NumParticles, 0.5);

template<template<typename> typename Layout>
auto createParticles(int numParticles) {

    static std::vector<InterleavedMemoryLayout2D::Members> members(numParticles);


    auto pRand = rng(0, 20, 1 << 20);
    auto vRand = rng(0, 10, 1 << 10);

    for(auto i = 0; i < numParticles; ++i){
        position[i] = {pRand(), pRand()};
        velocity[i] = {vRand(), vRand()};
    }

    if constexpr (std::is_same_v<Layout<glm::vec2>, InterleavedMemoryLayout2D>) {
        for(int i = 0; i < numParticles; i++){
            members[i].position = position[i];
            members[i].prePosition = position[i];
            members[i].velocity = velocity[i];
            members[i].inverseMass = inverseMass[i];
            members[i].restitution = restitution[i];
            members[i].radius = radius[i];
        }
        return createInterleavedMemoryParticle2D( members );
    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {
//        auto particles = createSeparateFieldParticle2D(numParticles);
//        for(auto i = 0; i < numParticles; ++i){
//            particles.position()[i] = position[i];
//            particles.previousPosition()[i] = position[i];
//            particles.velocity()[i] = velocity[i];
//            particles.inverseMass()[i] = inverseMass[i];
//            particles.restitution()[i] = restitution[i];
//            particles.radius()[i] = radius[i];
//        }
//        return particles;
        return createSeparateFieldParticle2D(position, velocity, inverseMass, restitution, radius);
    }
}


int main(int, char**){
//    World2D<SeparateFieldMemoryLayout> world{"physics world", {20, 20}, {1024, 1024}};
//    world.run();
    std::array<long long, 10000> runtimes{};
    auto particles = createParticles<SeparateFieldMemoryLayout>(NumParticles);
    SeparateFieldMemoryLayoutBasicSolver solver{particles, Bounds, NumParticles, 1.f, 1};

    for(int i = 0; i < runtimes.size(); i++){
        auto duration = profile<chrono::milliseconds>([&]{ solver.run(timeStep); });
        runtimes[i] = duration.count();
    }
    float average = std::accumulate(runtimes.begin(), runtimes.end(), 0);
    average /= to<float>(runtimes.size());
    fmt::print("solver average runtime {} ms", average);
}
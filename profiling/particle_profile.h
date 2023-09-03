#pragma once

#include "solver2d.h"
#include <benchmark/benchmark.h>
#include <glm/glm.hpp>
#include <random>
#include <tuple>

constexpr float timeStep = 0.00416666666;
constexpr int NumParticles = 50;
const Bounds2D bounds{glm::vec2(0), glm::vec2(20)};
using Range = std::tuple<float, float>;

auto rng(float low,  float high, uint32_t seed) {
    return
            [engine=std::default_random_engine{seed}
                    , dist=std::uniform_real_distribution<float>{low, high}]() mutable {
                return dist(engine);
            };
}

template<template<typename> typename Layout>
auto createParticles(int numParticles) {
    static std::vector<glm::vec2> position(numParticles);
    static std::vector<glm::vec2> velocity(numParticles);
    static std::vector<float> inverseMass(numParticles, 1);
    static std::vector<float> restitution(numParticles, 0.8);
    static std::vector<float> radius(numParticles, 0.5);
    static std::vector<InterleavedMemoryLayout2D::Members> members(numParticles);
    static std::vector<char> allocation(SeparateFieldMemoryLayout2D::allocationSize(numParticles));


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
        return createInterleavedMemoryParticle2DPtr( members );
    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {
//        auto particles = createSeparateFieldParticle2D(numParticles);
        auto particles = createSeparateFieldParticle2DPtr(allocation);
        for(auto i = 0; i < numParticles; ++i){
            particles->position()[i] = position[i];
            particles->previousPosition()[i] = position[i];
            particles->velocity()[i] = velocity[i];
            particles->inverseMass()[i] = inverseMass[i];
            particles->restitution()[i] = restitution[i];
            particles->radius()[i] = radius[i];
        }
        return particles;
//        return createSeparateFieldParticle2D(position, velocity, inverseMass, restitution, radius);
    }
}


static void BM_InterleavedMemoryLayoutSolver(benchmark::State& state){
    auto particles = createParticles<InterleavedMemoryLayout>(NumParticles);
    InterleavedMemoryLayoutBasicSolver solver{particles, bounds, 1.f};
    for(auto _ : state){
        solver.run(timeStep);
    }
}

static void BM_SeparateFieldMemoryLayoutSolver(benchmark::State& state){
    auto particles = createParticles<SeparateFieldMemoryLayout>(NumParticles);
    SeparateFieldMemoryLayoutBasicSolver solver{particles, bounds, 1.f};
    for(auto _ : state){
        solver.run(timeStep);
    }
}

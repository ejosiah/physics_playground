#pragma once

#include "model2d.h"
#include <glm/glm.hpp>
#include <utility>
#include "sdf2d.h"
#include "particle.h"
//const float Gravity{-9.8};
//
//enum class SolverType : uint32_t { Basic = 0 };
//

template<template<typename> typename Layout>
inline void boundsCheck(Particle2D<Layout>& particle, const Bounds2D& bounds, int i) {
    auto [min, max] = bounds;

    auto radius = particle.radius()[i];
    auto& position = particle.position()[i];
    auto& velocity = particle.velocity()[i];

    min += radius;
    max -= radius;
    auto& p = position;
    if(p.x < min.x){
        p.x = min.x;
        velocity.x *= -1;
    }
    if(p.x > max.x){
        p.x = max.x;
        velocity.x *= -1;
    }
    if(p.y < min.y){
        p.y = min.y;
        velocity.y *= -1;
    }
    if(p.y > max.y){
        p.y = max.y;
        velocity.y *= -1;
    }
}
//
//inline glm::vec2 toSdfBox(const Bounds2D& bounds){
//    auto [min, max] = bounds;
//    return max - min;
//}
//
//template<typename ParticleLayout, SolverType solverType>
//class Solver {
//public:
//    Solver() = default;
//
//    explicit Solver(ParticleLayout&, Bounds2D, uint32_t numSubSteps = 1)
//    {}
//
//    void solve(float dt){};
//};
//
//
//template<>
//class Solver<ArrayOfStructsLayout, SolverType::Basic> {
//public:
//    Solver() = default;
//
//    explicit Solver(ArrayOfStructsLayout& particles, Bounds2D bounds, uint32_t numSubSteps = 1)
//    : m_particles(particles)
//    , m_numSubSteps{numSubSteps}
//    , m_bounds{ toSdfBox(bounds) }
//    {}
//
//    void solve(float dt){
//        const auto N = m_numSubSteps;
//        float sdt = dt/static_cast<float>(N);
//        for(int i = 0; i < N; ++i){
//            subStep(sdt);
//        }
//    };
//
//    void subStep(float dt){
//        static auto const g = glm::vec2(0, Gravity);
//        for(auto& p : m_particles){
//            p.cPosition += p.velocity;
//            p.velocity += g * dt;
////            boundsCheck(p, m_bounds);
//        }
//    }
//
//protected:
//    uint32_t m_numSubSteps{1};
//    glm::vec2 m_bounds{};
//    ArrayOfStructsLayout m_particles{};
//};
#pragma once

#include "model2d.h"
#include "sdf2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include "snap.h"
#include "collision_handler.h"
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <span>
#include <utility>
#include <bitset>
#include <thread>

constexpr float Gravity{-9.8};

template<template<typename> typename Layout>
class Solver2D {
public:
    Solver2D() = default;

    Solver2D(std::shared_ptr<Particle2D<Layout>> particles
             , Bounds2D worldBounds
            , float maxRadius
            , int m_iterations = 1);

    virtual ~Solver2D() = default;

    void run(float dt);

    virtual void preSolve(float dt){}

    virtual void solve(float dt);

    virtual void postSolve(float dt) {}

    virtual void integrate(float dt) {};

    virtual void resolveCollision();

    CollisionHandler<Layout>::CollisionStats& collisionStats() {
        return m_collisionHandler.collisionStats;
    }

    CollisionHandler<Layout>& collisionHandler() {
        return m_collisionHandler;
    }

    inline void gravity(glm::vec2 value) {
        this->m_gravity = value;
    }

public:

protected:
    Bounds2D m_worldBounds;
    std::shared_ptr<Particle2D<Layout>> m_particles;
    int m_iterations{1};
    glm::vec2 m_gravity{0, -9.8};
    CollisionHandler<Layout> m_collisionHandler;
};


template<template<typename> typename Layout>
class VoidSolver : public Solver2D<Layout> {
public:
    VoidSolver() = default;
    ~VoidSolver() override = default;

    void solve(float dt) override {
        // DO Nothing
    }
};

template<template<typename> typename Layout>
class BasicSolver : public Solver2D<Layout> {
public:
    BasicSolver() = default;

    ~BasicSolver() final = default;

    BasicSolver(std::shared_ptr<Particle2D<Layout>>
            , Bounds2D worldBounds
            , float maxRadius
            , int iterations = 1);

    void integrate(float dt) final;

private:
    float damp{1.0};

};

template<template<typename> typename Layout>
class VarletIntegrationSolver : public Solver2D<Layout> {
public:
    VarletIntegrationSolver() = default;

    ~VarletIntegrationSolver() final = default;

    VarletIntegrationSolver(std::shared_ptr<Particle2D<Layout>>
            , Bounds2D worldBounds
            , float maxRadius
            , int iterations = 1);


    void postSolve(float dt) final;

    void integrate(float dt) final;
};


using SeparateFieldMemoryLayoutBasicSolver = BasicSolver<SeparateFieldMemoryLayout>;
using InterleavedMemoryLayoutBasicSolver = BasicSolver<InterleavedMemoryLayout>;

template<template<typename> typename Layout>
Solver2D<Layout>::Solver2D(std::shared_ptr<Particle2D<Layout>>  particles
        , Bounds2D worldBounds
        , float maxRadius
        , int iterations)
        : m_particles{ particles }
        , m_worldBounds{ worldBounds }
        , m_iterations{ iterations }
        , m_collisionHandler{particles, worldBounds, maxRadius}
{}

template<template<typename> typename Layout>
void Solver2D<Layout>::run(float dt) {
    const auto N = m_iterations;
    const auto sdt = dt/to<float>(N);

    auto size = m_particles->size();
    for(auto i = 0; i < N; ++i){
        solve(sdt);
    }
//    for(auto i = 0; i < N; i++){
//        m_collisionHandler.resolveCollision();
//    }
}

template<template<typename> typename Layout>
void Solver2D<Layout>::solve(float dt) {
    preSolve(dt);
    integrate(dt);
    resolveCollision();
    postSolve(dt);
}

template<template<typename> typename Layout>
void Solver2D<Layout>::resolveCollision() {
    m_collisionHandler.resolveCollision();
}

template<template<typename> typename Layout>
BasicSolver<Layout>::BasicSolver(std::shared_ptr<Particle2D<Layout>> particles, Bounds2D worldBounds, float maxRadius, int iterations)
: Solver2D<Layout>(particles, worldBounds, maxRadius, iterations)
{}

template<template<typename> typename Layout>
void BasicSolver<Layout>::integrate(float dt) {
    const auto N = this->m_particles->size();
    const glm::vec2 G = this->m_gravity;
    auto position = this->m_particles->position();
    auto velocity = this->m_particles->velocity();
#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        position[i] += velocity[i] * dt;
        velocity[i] = velocity[i] * glm::pow(damp, dt)  + G * dt;
    }
}

template<template<typename> typename Layout>
VarletIntegrationSolver<Layout>::VarletIntegrationSolver(std::shared_ptr<Particle2D<Layout>> particles
                                                         , Bounds2D worldBounds
                                                         , float maxRadius
                                                         , int iterations)
:Solver2D<Layout>(particles, worldBounds, maxRadius, iterations)
{}


template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::integrate(float dt) {
    const auto N = this->m_particles->size();
    const glm::vec2 G = this->m_gravity;

    auto position = this->m_particles->position();
    auto prevPosition = this->m_particles->previousPosition();

#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        auto p0 = prevPosition[i];
        auto p1 = position[i];
        auto p2 = 2.f * p1 - p0 + G * dt * dt;
        position[i] = p2;
        prevPosition[i] = p1;
    }
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::postSolve(float dt) {

}
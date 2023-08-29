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

    virtual void integrate(float dt) = 0;

    void resolveCollision();

    inline void gravity(glm::vec2 value) {
        this->m_gravity = value;
    }

public:
    struct CollisionStats{
        std::array<int, 100> average{};
        int max{0};
        int min{0};
        int next{0};
    };

    CollisionStats collisionStats{};

protected:
    Bounds2D m_worldBounds;
    std::shared_ptr<Particle2D<Layout>> m_particles;
    Particle2D<Layout>::Position m_position;
    Particle2D<Layout>::PreviousPosition m_prevPosition;
    Particle2D<Layout>::Velocity m_velocity;
    Particle2D<Layout>::InverseMass m_inverseMass;
    Particle2D<Layout>::Restitution m_restitution;
    Particle2D<Layout>::Radius m_radius;
    int m_iterations{1};
    float m_maxRadius{};
    glm::vec2 m_gravity{0, -9.8};
    CollisionHandler<Layout> m_collisionHandler;
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
        , m_position{ particles->position() }
        , m_prevPosition{ particles->previousPosition() }
        , m_velocity{ particles->velocity() }
        , m_inverseMass{ particles->inverseMass() }
        , m_restitution{ particles->restitution() }
        , m_radius{ particles->radius() }
        , m_worldBounds{ worldBounds }
        , m_maxRadius{ maxRadius }
        , m_iterations{ iterations }
        , m_collisionHandler{particles, worldBounds, maxRadius}
{}

template<template<typename> typename Layout>
void Solver2D<Layout>::run(float dt) {
    const auto N = m_iterations;
    const auto sdt = dt/to<float>(N);

    for(auto i = 0; i < N; ++i){
        solve(sdt);
    }
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
#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        this->m_position[i] += this->m_velocity[i] * dt;
        this->m_velocity[i] += G * dt;
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

    for(int i = 0; i < N; i++){
        auto p0 = this->m_prevPosition[i];
        auto p1 = this->m_position[i];
        auto p2 = 2.f * p1 - p0 + G * dt * dt;
        this->m_position[i] = p2;
        this->m_prevPosition[i] = p1;
        this->m_velocity[i] = (p2 - p1)/dt;
    }
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::postSolve(float dt) {
    const auto N = this->m_particles->size();

    for(int i = 0; i < N; i++){
        this->m_prevPosition[i] = this->m_position[i] - this->m_velocity[i] * dt;
    }
}
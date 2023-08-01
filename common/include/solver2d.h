#pragma once

#include "model2d.h"
#include <glm/glm.hpp>
#include <utility>
#include "sdf2d.h"
#include "particle.h"
#include <memory>
#include <span>
#include "spacial_hash.h"
#include <spdlog/spdlog.h>

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

constexpr float Gravity{-9.8};

struct Contact {
    int a{-1};
    int b{-1};
    float depth{0};
};

template<template<typename> typename Layout>
class ContactGenerator{
public:

    virtual std::span<Contact> generateContact(Particle2D<Layout>& m_particles) = 0;
};

template<template<typename> typename Layout>
class Solver2D {
public:
    Solver2D() = default;

    Solver2D(Particle2D<Layout> particles
             , Bounds2D worldBounds
            , int numParticles
            , float maxRadius
            , int m_iterations = 1);

    virtual ~Solver2D() = default;

    void run(float dt);

    virtual void preSolve(float dt){}

    virtual void solve(float dt);

    virtual void postSolve(float dt) {}

    virtual void integrate(float dt) = 0;

    void resolveCollision();

    void resolveCollision(int ia, int ib);

    void numParticles(int value) {
        assert(value <= 0);
        m_numParticles = value;
    }

public:
    struct CollisionStats{
        std::array<int, 100> average{};
        int max{std::numeric_limits<int>::min()};
        int min{std::numeric_limits<int>::max()};
        int next{0};
    };

    CollisionStats collisionStats{};

protected:
    Bounds2D m_worldBounds;
    Particle2D<Layout> m_particles;
    Particle2D<Layout>::Position m_position;
    Particle2D<Layout>::PreviousPosition m_prevPosition;
    Particle2D<Layout>::Velocity m_velocity;
    Particle2D<Layout>::InverseMass m_inverseMass;
    Particle2D<Layout>::Restitution m_restitution;
    Particle2D<Layout>::Radius m_radius;
    int m_iterations{1};
    int m_numParticles{0};
    float m_maxRadius{};
    SpacialHashGrid2D<> grid;
};


template<template<typename> typename Layout>
class BasicSolver : public Solver2D<Layout> {
public:
    BasicSolver() = default;

    ~BasicSolver() final = default;

    BasicSolver(Particle2D<Layout> particles
            , Bounds2D worldBounds
            , int numParticles
            , float maxRadius
            , int iterations = 1);

    void integrate(float dt) final;
};

template<template<typename> typename Layout>
class VarletIntegrationSolver : public Solver2D<Layout> {
public:
    VarletIntegrationSolver() = default;

    ~VarletIntegrationSolver() final = default;

    VarletIntegrationSolver(Particle2D<Layout> particles
    , Bounds2D worldBounds
    , int numParticles
    , float maxRadius
    , int iterations = 1);


    void postSolve(float dt) final;

    void integrate(float dt) final;
};


using SeparateFieldMemoryLayoutBasicSolver = BasicSolver<SeparateFieldMemoryLayout>;
using InterleavedMemoryLayoutBasicSolver = BasicSolver<InterleavedMemoryLayout>;


template<template<typename> typename Layout>
Solver2D<Layout>::Solver2D(Particle2D<Layout> particles
                           , Bounds2D worldBounds
                           , int numParticles
                           , float maxRadius
                           , int iterations)
: m_particles{ particles }
, m_position{ particles.position() }
, m_prevPosition{ particles.previousPosition() }
, m_velocity{ particles.velocity() }
, m_inverseMass{ particles.inverseMass() }
, m_restitution{ particles.restitution() }
, m_radius{ particles.radius() }
, m_worldBounds{ worldBounds }
, m_maxRadius{ maxRadius }
, m_numParticles{ numParticles }
, m_iterations{ iterations }
{
    grid = SpacialHashGrid2D{m_maxRadius * 2, to<int32_t>(particles.size()) };
}

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
    grid.initialize(m_particles, m_numParticles);

    auto& vPositions = m_position;
    for(int i = 0; i < m_numParticles; i++){
        auto& position = vPositions[i];
        auto ids = grid.query(position, glm::vec2(m_maxRadius * 2));

        int collisions = 0;
        for(int j : ids){
            if(i == j) continue;
            collisions++;
            resolveCollision(i, j);
        }
        boundsCheck(m_particles, m_worldBounds, i);

        collisionStats.average[collisionStats.next++] = collisions;
        collisionStats.max = glm::max(collisionStats.max, collisions);
        collisionStats.min = glm::min(collisionStats.min, collisions);

        collisionStats.next %= collisionStats.average.size();
    }
}

template<template<typename> typename Layout>
void Solver2D<Layout>::resolveCollision(int ia, int ib) {
    auto& position = m_position;
    auto& velocity = m_velocity;
    auto& inverseMass = m_inverseMass;
    auto& restitution = m_restitution;
    auto& radius = m_radius;

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto rr = radius[ia] + radius[ib];
    rr *= rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr) return;

    auto d = glm::sqrt(dd);
    dir /= d;

    auto corr = (radius[ib] + radius[ia] - d) * .5f;
    pa -= dir * corr;
    pb += dir * corr;

    auto v1 = glm::dot(velocity[ia], dir);
    auto v2 = glm::dot(velocity[ib], dir);

    auto m1 = 1/inverseMass[ia];
    auto m2 = 1/inverseMass[ib];

    auto newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution[ia]) / (m1 + m2);
    auto newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution[ib]) / (m1 + m2);

    velocity[ia] += dir * (newV1 - v1);
    velocity[ib] += dir * (newV2 - v2);
}

template<template<typename> typename Layout>
BasicSolver<Layout>::BasicSolver(Particle2D<Layout> particles, Bounds2D worldBounds, int numParticles, float maxRadius, int iterations)
: Solver2D<Layout>(particles, worldBounds, numParticles, maxRadius, iterations)
{}

template<template<typename> typename Layout>
void BasicSolver<Layout>::integrate(float dt) {
    const auto N = this->m_numParticles;
    const glm::vec2 G = glm::vec2{0, -9.8};
#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        this->m_position[i] += this->m_velocity[i] * dt;
        this->m_velocity[i] += G * dt;
    }
}

template<template<typename> typename Layout>
VarletIntegrationSolver<Layout>::VarletIntegrationSolver(Particle2D<Layout> particles, Bounds2D worldBounds,
                                                         int numParticles, float maxRadius, int iterations)
:Solver2D<Layout>(particles, worldBounds, numParticles, maxRadius, iterations)
{}


template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::integrate(float dt) {
    const auto N = this->m_numParticles;
    const glm::vec2 G = glm::vec2{0, -9.8};

    for(int i = 0; i < N; i++){
        auto p0 = this->m_prevPosition[i];
        auto p1 = this->m_position[i];
        auto p2 = 2 * p1 - p0 + G * dt * dt;
        this->m_position[i] = p2;
        this->m_prevPosition[i] = p1;
        this->m_velocity[i] = (p2 - p1)/dt;
    }
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::postSolve(float dt) {
    const auto N = this->m_numParticles;

    for(int i = 0; i < N; i++){
        this->m_prevPosition[i] = this->m_position[i] - this->m_velocity[i] * dt;
    }
}
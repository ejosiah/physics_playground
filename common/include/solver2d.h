#pragma once

#include "model2d.h"
#include "sdf2d.h"
#include "particle.h"
#include "spacial_hash.h"
#include "snap.h"
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <span>
#include <utility>
#include <bitset>
#include <thread>

constexpr float Gravity{-9.8};

template<template<typename> typename Layout>
inline void boundsCheck(Particle2D<Layout>& particle, const Bounds2D& bounds, int i) {
    auto radius = particle.radius()[i];
    auto& position = particle.position()[i];
    auto& velocity = particle.velocity()[i];
    bool collides = false;

    auto [min, max] = shrink(bounds, radius);

    auto& p = position;
    if(p.x < min.x){
        p.x = min.x;
        velocity.x *= -1;
        collides = true;
    }
    if(p.x > max.x){
        p.x = max.x;
        velocity.x *= -1;
        collides = true;
    }
    if(p.y < min.y){
        p.y = min.y;
        velocity.y *= -1;
        // remove velocity caused by gravity
        // FIXME remove magic number and pass in gravity and frame rate
//        velocity.y -= 0.163333f;
//        velocity.y = glm::max(0.f, velocity.y);
        collides = true;
    }
    if(p.y > max.y){
        p.y = max.y;
        velocity.y *= -1;
        collides = true;
    }
//    if(collides){
//        boundCollisions.push_back(i);
//    }
}

inline bool contains(const Bounds2D& bounds, const glm::vec2& p){
    auto [min, max] = bounds;
    return
            p.x >= min.x && p.x < max.x &&
            p.y >= min.y && p.y < max.y;
}

struct CollisionStats{
    std::array<int, 100> average{};
    int max{0};
    int min{0};
    int next{0};
    int total{0};
};

template<template<typename> typename Layout>
class Solver2D {
public:
    Solver2D() = default;

    Solver2D(std::shared_ptr<Particle2D<Layout>> particles
    , Bounds2D worldBounds);

    virtual ~Solver2D() = default;

    [[nodiscard]]
    const Bounds2D& bounds() const {
        return m_worldBounds;
    }

    Particle2D<Layout>& particles() {
        return *m_particles;
    }

    [[nodiscard]]
    const glm::vec2& gravity() const {
        return m_gravity;
    }

    virtual void solve(float dt) = 0;

    virtual void clear() {}

public:
    CollisionStats collisionStats{};

protected:
    Bounds2D m_worldBounds;
    std::shared_ptr<Particle2D<Layout>> m_particles;
    glm::vec2 m_gravity{0, -9.8};
};

template<template<typename> typename Layout>
Solver2D<Layout>::Solver2D(std::shared_ptr<Particle2D<Layout>>  particles
        , Bounds2D worldBounds)
        : m_particles{ particles }
        , m_worldBounds{ worldBounds }
{}




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
class ExplicitEulerSolver : public Solver2D<Layout> {
public:
    ExplicitEulerSolver(std::shared_ptr<Particle2D<Layout>>
            , Bounds2D worldBounds
            , float maxRadius
            , int iterations = 1);


    ~ExplicitEulerSolver() override = default;

    void solve(float dt) override;

    void subStep(float dt);

    void integrate(float dt);

    void resolveCollision(float dt);

    int resolveCollision(int ia, int ib);

    void boundsCheck(int i);

private:
    BoundedSpacialHashGrid2D m_grid;
    int m_iterations{1};
    float m_damp{1.0};
    float m_radius;
};


template<template<typename> typename Layout = SeparateFieldMemoryLayout>
class VarletIntegrationSolver : public Solver2D<Layout> {
public:
    VarletIntegrationSolver() = default;

    ~VarletIntegrationSolver() final = default;

    VarletIntegrationSolver(std::shared_ptr<Particle2D<Layout>>
            , Bounds2D worldBounds
            , float maxRadius
            , int iterations = 1);


    void solve(float dt) override;

    void subStep(float dt);

    void integrate(float dt);

    void resolveCollision(float dt);

    int resolveCollision(int ia, int ib);

    void boundsCheck(int i);

    std::unordered_map<int, std::set<int>>& hashCollisions() {
        return m_grid.m_collisions;
    }

private:
    UnBoundedSpacialHashGrid2D m_grid;
    int m_iterations{1};
    float m_damp{1};
    float m_radius{1};
};


template<template<typename> typename Layout>
ExplicitEulerSolver<Layout>::ExplicitEulerSolver(std::shared_ptr<Particle2D<Layout>> particles, Bounds2D worldBounds, float maxRadius, int iterations)
        : Solver2D<Layout>(particles, worldBounds)
        , m_iterations(iterations)
        , m_radius(maxRadius)
{
    glm::ivec2 gridSize = worldBounds.upper - worldBounds.lower;
    m_grid = BoundedSpacialHashGrid2D{maxRadius * 2, gridSize };
}

template<template<typename> typename Layout>
void ExplicitEulerSolver<Layout>::solve(float dt) {
    const auto sdt = dt/to<float>(m_iterations);
    for(auto i = 0; i < m_iterations; i++){
        subStep(sdt);
    }
}
template<template<typename> typename Layout>
void ExplicitEulerSolver<Layout>::subStep(float dt) {
    resolveCollision(dt);
    integrate(dt);
}



template<template<typename> typename Layout>
void ExplicitEulerSolver<Layout>::integrate(float dt){
    auto& particles = this->particles();
    const auto N = particles.size();
    const glm::vec2 G = this->m_gravity;
    auto position = particles.position();
    auto velocity = particles.velocity();

#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        position[i] += velocity[i] * dt;
        velocity[i] = velocity[i] * glm::pow(m_damp, dt)  + G * dt;
    }
}

template<template<typename> typename Layout>
void ExplicitEulerSolver<Layout>::resolveCollision(float dt){
    const auto numParticles = this->particles().size();
    auto vPositions = this->particles().position();
    m_grid.initialize(this->particles(), numParticles);

    for(int i = 0; i < numParticles; i++){
        auto& position = vPositions[i];

        auto ids = m_grid.query(position, glm::vec2(m_radius * 2));

        int collisions = 0;
        for(int j : ids){
            if(i == j) continue;
            collisions += resolveCollision(i, j);
        }
        this->collisionStats.average[this->collisionStats.next++] = collisions;
        this->collisionStats.max = glm::max(this->collisionStats.max, collisions);
        this->collisionStats.min = glm::min(this->collisionStats.min, collisions);

        this->collisionStats.next %= this->collisionStats.average.size();
        this->collisionStats.total += collisions;
    }
    for(auto i = 0; i < numParticles; i++){
        boundsCheck(i);
    }
}

template<template<typename> typename Layout>
void ExplicitEulerSolver<Layout>::boundsCheck(int i) {
    auto radius = this->particles().radius()[i];
    auto& position = this->particles().position()[i];
    auto& velocity = this->particles().velocity()[i];
    auto rest = this->particles().restitution()[i];

    bool collides = false;

    auto [min, max] = shrink(this->bounds(), radius);

    auto& p = position;
    glm::vec2 n{0};

    if(p.x < min.x){
        p.x = min.x;
        velocity.x *= -rest;
        collides = true;
    }
    if(p.x > max.x){
        p.x = max.x;
        velocity.x *= -rest;
        collides = true;
    }
    if(p.y < min.y){
        p.y = min.y;
        velocity.y *= -rest;
        // remove velocity caused by gravity
        // FIXME remove magic number and pass in gravity and frame rate
//        velocity.y -= 0.163333f;
//        velocity.y = glm::max(0.f, velocity.y);
        collides = true;
    }
    if(p.y > max.y){
        p.y = max.y;
        velocity.y *= -rest;
        collides = true;
    }

    auto v = glm::dot(velocity, n);

//    if(collides){
//        boundCollisions.push_back(i);
//    }
}

template<template<typename> typename Layout>
int ExplicitEulerSolver<Layout>::resolveCollision(int ia, int ib){
    auto position = this->particles().position();
    auto velocity = this->particles().velocity();
    auto inverseMass = this->particles().inverseMass();
    auto restitution = this->particles().restitution();
    auto radius = this->particles().radius();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto rr = radius[ia] + radius[ib];
    rr *= rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr) return 0;

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

    return 1;
}


template<template<typename> typename Layout>
VarletIntegrationSolver<Layout>::VarletIntegrationSolver(std::shared_ptr<Particle2D<Layout>> particles, Bounds2D worldBounds, float maxRadius, int iterations)
        : Solver2D<Layout>(particles, worldBounds)
        , m_iterations(iterations)
        , m_radius(maxRadius)
{
    glm::ivec2 gridSize = worldBounds.upper - worldBounds.lower;
   // m_grid = BoundedSpacialHashGrid2D{maxRadius * 2, gridSize };
    m_grid = UnBoundedSpacialHashGrid2D{maxRadius * 2, 20000 };
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::solve(float dt) {
    const auto sdt = dt/to<float>(m_iterations);
    for(auto i = 0; i < m_iterations; i++){
        subStep(sdt);
    }
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::subStep(float dt) {
    resolveCollision(dt);
    integrate(dt);
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::resolveCollision(float dt) {
//    this->particles().sort([this](const auto& a, const auto& b){
//        return m_grid.hashPosition(this->particles().position()[a]) <= m_grid.hashPosition(this->particles().position()[b]);
//    });
    const auto numParticles = this->particles().size();

    auto vPositions = this->particles().position();

    m_grid.initialize(this->particles(), numParticles);

    for(int i = 0; i < numParticles; i++){
        auto& position = vPositions[i];

        auto ids = m_grid.query(position, glm::vec2(m_radius * 2));

        int collisions = 0;
        for(int j : ids){
            if(i == j) continue;
            collisions += resolveCollision(i, j);
        }
        this->collisionStats.average[this->collisionStats.next++] = collisions;
        this->collisionStats.max = glm::max(this->collisionStats.max, collisions);
        this->collisionStats.min = glm::min(this->collisionStats.min, collisions);

        this->collisionStats.next %= this->collisionStats.average.size();
        this->collisionStats.total += collisions;
    }
    for(auto i = 0; i < numParticles; i++){
        boundsCheck(i);
    }
}

template<template<typename> typename Layout>
int VarletIntegrationSolver<Layout>::resolveCollision(int ia, int ib) {
    auto position = this->particles().position();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    constexpr auto rr = 0.2f;
    constexpr auto rr2 = rr * rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr2) return 0;

    auto d = glm::sqrt(dd);
    dir /= d;

    auto corr = 0.5f * (rr - d) * .5f;
    pa -= dir * corr;
    pb += dir * corr;

    return 1;
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::boundsCheck(int i) {
    auto radius = this->particles().radius()[i];
    auto& position = this->particles().position()[i];

    auto [min, max] = shrink(this->bounds(), radius);

    auto& p = position;
    glm::vec2 n{0};
    glm::vec2 d{0};

    if(p.x < min.x){
        p.x = min.x;
    }
    if(p.x > max.x){
        p.x = max.x;
    }
    if(p.y < min.y){
        p.y = min.y;
    }
    if(p.y > max.y){
        p.y = max.y;
    }
}

template<template<typename> typename Layout>
void VarletIntegrationSolver<Layout>::integrate(float dt) {
    const auto N = this->m_particles->size();
    const glm::vec2 G = this->m_gravity;

    auto position = this->m_particles->position();
    auto prevPosition = this->m_particles->previousPosition();
    auto velocity = this->m_particles->velocity();

#pragma loop(hint_parallel(8))
    for(int i = 0; i < N; i++){
        auto p0 = prevPosition[i];
        auto p1 = position[i];
        auto p2 = 2.f * p1 - p0 + G * dt * dt;
        position[i] = p2;
        prevPosition[i] = p1;
        velocity[i] = (p2 - p1)/dt;
    }
}



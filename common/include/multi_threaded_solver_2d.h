#pragma once

#include "solver2d.h"
#include "thread_pool/thread_pool.hpp"
#include <vector>

template<template<typename> typename Layout>
class MultiThreadedSolver : public Solver2D<Layout> {
public:
    MultiThreadedSolver(
            std::shared_ptr<Particle2D<Layout>> particles,
            Bounds2D worldBounds,
            float maxRadius,
            int iterations = 1,
            int numThreads = 1);

    void solve(float dt) override;

    void subStep(float dt);

    void integrate(float dt);

    void resolveCollision(float dt);

    int resolveCollision(int ia, int ib);

    void boundsCheck(int i);
    
    void workerThreadResolveCollision(int id, int pass);

private:
    BoundedSpacialHashGrid2D m_grid;
    int m_iterations{1};
    float m_damp{1.0};
    float m_radius;
    tp::ThreadPool m_threadPool;
    std::vector<glm::vec2> m_threadLocalParticles;
};


template<template<typename> typename Layout>
MultiThreadedSolver<Layout>::MultiThreadedSolver(std::shared_ptr<Particle2D<Layout>> particles, Bounds2D worldBounds,
                                                 float maxRadius, int iterations, int numThreads)
        : Solver2D<Layout>(particles, worldBounds)
        , m_iterations(iterations)
        , m_radius(maxRadius)
        , m_threadPool(numThreads)
{
    glm::ivec2 gridSize = worldBounds.upper - worldBounds.lower;
    m_grid = BoundedSpacialHashGrid2D{maxRadius * 2, gridSize };
}

template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::solve(float dt) {
    const auto sdt = dt/to<float>(m_iterations);
    for(auto i = 0; i < m_iterations; i++){
        subStep(sdt);
    }
}

template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::subStep(float dt) {
    resolveCollision(dt);
    integrate(dt);
}



template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::integrate(float dt) {
    const auto N = this->particles().size();
    const glm::vec2 G = this->m_gravity;

    auto position = this->particles().position();
    auto prevPosition = this->particles().previousPosition();
    auto velocity = this->particles().velocity();

    m_threadPool.dispatch(N, [&](const auto start, const auto end){
        for(int i = start; i < end; i++){
            auto p0 = prevPosition[i];
            auto p1 = position[i];
            auto p2 = 2.f * p1 - p0 + G * dt * dt;
            position[i] = p2;
            prevPosition[i] = p1;
            velocity[i] = (p2 - p1)/dt;
        }
    });
}


template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::resolveCollision(float dt) {
    m_grid.initialize(this->particles(), this->particles().size());
    for(auto i = 0; i < m_threadPool.m_thread_count; i++) {
        m_threadPool.addTask([i, this]{ workerThreadResolveCollision(i, 0); });
    }
    m_threadPool.waitForCompletion();

    for(auto i = 0; i < m_threadPool.m_thread_count; i++) {
        m_threadPool.addTask([i, this]{ workerThreadResolveCollision(i, 1); });
    }
    m_threadPool.waitForCompletion();

    m_threadPool.dispatch(this->particles().size(), [this](const auto start, const auto end) {
        for (auto i = start; i < end; i++) {
            boundsCheck(i);
        }
    });
}

template<template<typename> typename Layout>
int MultiThreadedSolver<Layout>::resolveCollision(int ia, int ib) {
    auto position = this->particles().position();
    auto restitution = this->particles().restitution();
    auto radius = this->particles().radius();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto rr = radius[ia] + radius[ib];
    auto rr2 = rr * rr;
    auto dd = glm::dot(dir, dir);
    if(dd == 0 || dd > rr2) return 0;

    auto d = glm::sqrt(dd);
    dir /= d;

    auto corr = restitution[ia] * (rr - d) * .5f;
    pa -= dir * corr;
    pb += dir * corr;

    return 1;
}

template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::boundsCheck(int i) {
    auto radius = this->particles().radius()[i];
    auto& position = this->particles().position()[i];
    auto rest = this->particles().restitution()[i];

    bool collides = false;

    auto [min, max] = shrink(this->bounds(), radius);

    auto& p = position;
    glm::vec2 n{0};
    glm::vec2 d{0};

    if(p.x < min.x){
        n.x = -1;
        d.x = min.x - p.x;
        collides = true;
    }
    if(p.x > max.x){
        n.x = 1;
        d.x = p.x - max.x;
        collides = true;
    }
    if(p.y < min.y){
        n.y = -1;
        d.y = min.y - p.y;
        collides = true;
    }
    if(p.y > max.y){
        n.y = 1;
        d.y = p.y - max.y;
        collides = true;
    }

    if(collides) {
        p -= glm::normalize(n) * glm::length(d) * rest;
    }
//    if(collides){
//        boundCollisions.push_back(i);
//    }
}


template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::workerThreadResolveCollision(int id, int pass) {
    auto gridSpacing = this->m_radius * 2;
    const auto numParticles = this->particles().size();
    const auto numWorkers = m_threadPool.m_thread_count;

    auto gridSize = this->bounds().upper - this->bounds().lower;
    auto [sizeX, sizeY] = dimensions(this->bounds());
    auto ratio = 1.f/to<float>(numWorkers);
    Bounds2D bounds = this->bounds();
    auto offset = sizeX * ratio;
    bounds.lower.x += to<float>(id) * offset;
    bounds.upper.x = bounds.lower.x + offset;

    sizeX = bounds.upper.x - bounds.lower.x;
    bounds.lower.x += pass * sizeX/2;
    bounds.upper.x = bounds.lower.x + sizeX/2;

//        spdlog::info("worker({}, pass: {}) working on bounds({}, {})", id, pass + 1, bounds.lower, bounds.upper);
    auto vPositions = this->particles().position();

    for(int i = 0; i < numParticles; i++){

        auto& position = vPositions[i];
        if(!contains(bounds, position)){
            continue;
        }

        threadGroup[pass][id].push_back(i);

        auto ids = m_grid.query(position, glm::vec2(m_radius * 2));

        int collisions = 0;
        for(int j : ids){
            if(i == j) continue;
            collisions += resolveCollision(i, j);
        }
//            collisionStats.average[collisionStats.next++] = collisions;
//            collisionStats.max = glm::max(collisionStats.max, collisions);
//            collisionStats.min = glm::min(collisionStats.min, collisions);
//
//            collisionStats.next %= collisionStats.average.size();
//            collisionStats.total += collisions;
    }
//        if(pass == 1){
//            processed.clear();
//        }
}
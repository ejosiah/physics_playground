#pragma once

#include "solver2d.h"
#include "thread_pool/thread_pool.hpp"
#include <vector>
#include <istream>
#include <mutex>

template<template<typename> typename Layout>
class CollisionResolver;

template<template<typename> typename Layout>
class MultiThreadedSolver : public Solver2D<Layout> {
public:
    friend class CollisionResolver<Layout>;
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
    
    void workerThreadResolveCollision(int id);

private:
    UnBoundedSpacialHashGrid2D m_grid;
    int m_iterations{1};
    float m_damp{1.0};
    float m_radius;
    tp::ThreadPool m_threadPool;
    std::vector<glm::vec2> m_threadLocalParticles;
    std::vector<CollisionResolver<Layout>> m_resolvers;
    std::mutex ghost_mutex;
    std::condition_variable ghost_cv;
    int ghost_id{-1};
};

template<template<typename> typename Layout>
class CollisionResolver {
public:
    CollisionResolver(uint32_t id, MultiThreadedSolver<Layout>& solver)
    : m_id(id)
    , m_solver(&solver)
    , m_gridSpacing(solver.m_radius * 2)
    , m_numWorkers(solver.m_threadPool.m_thread_count)
    {
        auto [sizeX, sizeY] = dimensions(solver.bounds());
        auto ratio = 1.f/to<float>(solver.m_threadPool.m_thread_count);
        m_bounds = solver.bounds();
        auto offset = sizeX * ratio;
        m_bounds.lower.x += to<float>(id) * offset;
        m_bounds.upper.x = m_bounds.lower.x + offset;
        std::stringstream ss;
        ss << fmt::format("worker({}) bounds({}, {}) ", m_id, m_bounds.lower, m_bounds.upper);

        if(m_numWorkers > 1){
            auto g = solver.m_radius * 2;
            if(id != 0) {   // left ghost region
                ss << fmt::format("lg region: [{},{}] ", m_bounds.lower.x - g, m_bounds.lower.x);
                m_bounds.lower.x -= g;
            }
            if(id != m_numWorkers - 1) {  // right ghost region
                ss << fmt::format("rg region: [{},{}] ",m_bounds.upper.x, m_bounds.upper.x + g);
                m_bounds.upper.x += g;
            }
        }
        spdlog::info("{}", ss.str());
    }

    [[nodiscard]]
    bool isGhost(const glm::vec2& p) const {
        if(m_numWorkers == 1) return false;
        const auto g = m_gridSpacing;

        bool leftGhost = m_id > 0 && p.x > m_bounds.lower.x && p.x <= m_bounds.lower.x + g; // left ghost region

        if(leftGhost){
//            spdlog::info("worker({}): {} is in left ghost region {}", m_id, p, m_bounds.lower.x);
        }

        bool rightGhost = m_id < (m_numWorkers - 1) && p.x > m_bounds.upper.x - g &&  p.x <= m_bounds.upper.x; // right ghost region
//        if(rightGhost){
//            spdlog::info("worker({}): {} is in right ghost region  [{},{}]", m_id, p.x, m_bounds.upper.x - g, m_bounds.upper.x);
//        }
        return leftGhost || rightGhost;
    }

    void resolve() {
        const auto numParticles = m_solver->particles().size();

//    spdlog::info("worker({}) working on bounds({}, {})", m_id, m_bounds.lower, m_bounds.upper);
        auto vPositions = m_solver->particles().position();

        for(int i = 0; i < numParticles; i++){

            auto& position = vPositions[i];

            if(isGhost(position)) {
                threadGroup[0][m_id].push_back(i);
            }

            if(isGhost(position) || !contains(m_bounds, position)){
                continue;
            }

            auto ids = m_solver->m_grid.query(position, glm::vec2(m_gridSpacing));

            int collisions = 0;
            for(int j : ids){
                if(i == j) continue;
                auto& pa = position;
                auto& pb = vPositions[j];

                glm::vec2 dir = pb - pa;
                constexpr auto rr = 0.2f;
                constexpr auto rr2 = rr * rr;
                auto dd = glm::dot(dir, dir);
                auto collides = !(dd == 0 || dd > rr2);

                if(collides) {
                    auto d = glm::sqrt(dd);
                    dir /= d;

                    auto corr = 0.5f * (rr - d) * .5f;
                    pa -=  dir * corr;
                    pb += !isGhost(pb) ? dir * corr : glm::vec2(0);
                    collisions++;
                }
            }
//            m_solver->collisionStats.average[m_solver->collisionStats.next++] = collisions;
//            m_solver->collisionStats.max = glm::max(m_solver->collisionStats.max, collisions);
//            m_solver->collisionStats.min = glm::min(m_solver->collisionStats.min, collisions);
//
//            m_solver->collisionStats.next %= m_solver->collisionStats.average.size();
//            m_solver->collisionStats.total += collisions;
        }
    }

private:
    uint32_t m_id;
    Bounds2D m_bounds;
    float m_gridSpacing;
    uint32_t m_numWorkers;
    MultiThreadedSolver<Layout>* m_solver;
    static thread_local std::vector<glm::vec2> local_positions;
    static thread_local std::vector<glm::vec2> local_ids;
    static thread_local size_t local_numParticles;
};

template<template<typename> typename Layout>
thread_local std::vector<glm::vec2> local_positions{};
static thread_local std::vector<glm::vec2> local_ids{};
static thread_local  size_t local_numParticles{};


template<template<typename> typename Layout>
MultiThreadedSolver<Layout>::MultiThreadedSolver(std::shared_ptr<Particle2D<Layout>> particles, Bounds2D worldBounds,
                                                 float maxRadius, int iterations, int numThreads)
        : Solver2D<Layout>(particles, worldBounds)
        , m_iterations(iterations)
        , m_radius(maxRadius)
        , m_threadPool(numThreads)
{
    m_grid = UnBoundedSpacialHashGrid2D{maxRadius * 2, 20000 };
    for(uint32_t i = 0; i < numThreads; i++){
        m_resolvers.push_back({i, *this});
    }
    g_numThreads = numThreads;
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

//    m_threadPool.dispatch(N, [&](const auto start, const auto end){
        for(int i = 0; i < N; i++){
            auto p0 = prevPosition[i];
            auto p1 = position[i];
            auto p2 = 2.f * p1 - p0 + G * dt * dt;
            position[i] = p2;
            prevPosition[i] = p1;
            velocity[i] = (p2 - p1)/dt;
        }
//    });
}


template<template<typename> typename Layout>
void MultiThreadedSolver<Layout>::resolveCollision(float dt) {
    const auto numParticles = this->particles().size();
    m_grid.initialize(this->particles(), this->particles().size());
    for(auto i = 0; i < m_threadPool.m_thread_count; i++) {
        m_threadPool.addTask([i, this]{ m_resolvers[i].resolve(); });
    }
    m_threadPool.waitForCompletion();


//    m_threadPool.dispatch(this->particles().size(), [this](const auto start, const auto end) {
        for (auto i = 0; i < numParticles; i++) {
            boundsCheck(i);
        }
//    });
}

template<template<typename> typename Layout>
int MultiThreadedSolver<Layout>::resolveCollision(int ia, int ib) {
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
void MultiThreadedSolver<Layout>::boundsCheck(int i) {
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
void MultiThreadedSolver<Layout>::workerThreadResolveCollision(int id) {
    auto gridSpacing = this->m_radius * 2;
    const auto numParticles = this->particles().size();
    const auto numWorkers = m_threadPool.m_thread_count;

//    auto gridSize = this->bounds().upper - this->bounds().lower;
//    auto [sizeX, sizeY] = dimensions(this->bounds());
//    auto ratio = 1.f/to<float>(numWorkers);
//    Bounds2D bounds = this->bounds();
//    auto offset = sizeX * ratio;
//    bounds.lower.x += to<float>(id) * offset;
//    bounds.upper.x = bounds.lower.x + offset;


//    spdlog::info("worker({}) working on bounds({}, {})", id, bounds.lower, bounds.upper);
    auto vPositions = this->particles().position();

    for(int i = 0; i < numParticles; i++){

        auto& position = vPositions[i];
//        if(!contains(bounds, position)){
//            continue;
//        }

//        threadGroup[0][id].push_back(i);

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
}
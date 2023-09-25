#pragma once

#include "solver2d.h"
#include "sph.h"
#include "spacial_hash.h"
#include <functional>

template<template<typename> typename Layout>
class SphSolver2D : public Solver2D<Layout> {
public:

    SphSolver2D() = default;

    SphSolver2D(
            Kernel2D kernel,
            float smoothingRadius,
            float particleRadius,
            float gasConstant,
            float viscousConstant,
            float gravity,
            float mass,
            size_t maxNumParticles,
            std::shared_ptr<Particle2D<Layout>> particles,
            Bounds2D worldBounds,
            size_t numIterations)
            : Solver2D<Layout>(particles, worldBounds)
            , m_kernel(kernel)
            , m_smoothingRadius( smoothingRadius )
            , m_radius(particleRadius)
            , m_gasConstant( gasConstant )
            , m_viscousConstant( viscousConstant )
            , m_gravity( gravity )
            , m_mass( mass )
            , m_numIterations( numIterations )
            , m_gravityForce(0, -gravity)
            , m_density(maxNumParticles)
            , m_forces(maxNumParticles)
            , W(kernel(smoothingRadius * 2))
            , dW(kernel.gradient(smoothingRadius * 2))
            , ddW(kernel.laplacian(smoothingRadius * 2))
            , m_grid{ smoothingRadius, 25000}
            , h4(std::pow(smoothingRadius * 2, 4))
{}

    ~SphSolver2D() override = default;

    void solve(float dt) override {
        const auto sdt = dt/to<float>(m_numIterations);
        for(auto i = 0; i < m_numIterations; i++){
            subStep(sdt);
        }
    }

    void clear() override {
        std::fill_n(m_density.begin(), m_density.size(), 0.f);
        std::fill_n(m_forces.begin(), m_forces.size(), glm::vec2(0));
    }

    void subStep(float dt) {

        const auto N = this->particles().size();
        m_grid.initialize(this->particles(), N);
        const auto h = glm::vec2(m_smoothingRadius * 2);

        resolveCollision(N, dt);
        computeDensity(N, h);
        computeForces(N, h);
        integrate(N, dt);
    }

    void resolveCollision(size_t N, float dt) {
        for(auto i = 0; i < N; i++){
            boundsCheck(i);
        }
    }

    void boundsCheck(int i) {
        auto& position = this->particles().position()[i];
        auto& velocity = this->particles().velocity()[i];
        auto rest = 0.5f;

        auto [min, max] = shrink(this->bounds(), m_radius);

        auto& p = position;
        glm::vec2 n{0};

        if(p.x < min.x){
            p.x = min.x;
            velocity.x *= -rest;
        }
        if(p.x > max.x){
            p.x = max.x;
            velocity.x *= -rest;
        }
        if(p.y < min.y){
            p.y = min.y;
            velocity.y *= -rest;
        }
        if(p.y > max.y){
            p.y = max.y;
            velocity.y *= -rest;
        }
    }

    void computeDensity(size_t N, glm::vec2 h) {
        float d;
        for(auto i = 0; i < N; i++){
            auto p = this->particles().position()[i];
            auto neighbours = m_grid.query(p, h);
            m_density[i] = computeDensity(i, neighbours);
            d = m_density[i];
        }
//        spdlog::info("density: {}", d);
    }

    float computeDensity(int i, std::span<int> neighbours) {
        const auto N = neighbours.size();
        auto xi = this->particles().position()[i];

        float totalWeight = 0;
        for(auto j : neighbours) {
            auto xj = this->particles().position()[j];
            totalWeight += W(xi - xj);
        }

        return m_mass * N * totalWeight;
    }

    void computeForces(size_t N, glm::vec2 h) {
        for(auto i = 0; i < N; i++){
            auto& f = m_forces[i];
            f.x = f.y = 0;
            auto density = m_density[i];
            auto x = this->particles().position()[i];
            auto neighbours = m_grid.query(x, h);
            computePressureForce(i, neighbours, x, density, f);
//            if(i == N/2) {
//                spdlog::info("pressure force: {}", f);
//            }
            f += m_gravityForce + computeViscousForce(i, neighbours);
        }
    }

    void computePressureForce(int i, std::span<int> neighbours, const glm::vec2& xi, float di, glm::vec2& f) {
        const auto k = m_gasConstant;
        const auto m = m_mass;

        if(di == 0 ) return;

        for(auto j : neighbours){
            if(j == i) continue;
            auto xj = this->particles().position()[j];
            auto dj = m_density[j];
            auto r = xi - xj;
            auto w = dW(r);
            f +=  (dj == 0) ? glm::vec2{0} : (m/dj) * k * (di + dj) * w * 0.5f;
        }
        f *= -(m/di);
    }


    glm::vec2 computeViscousForce(int i, std::span<int> neighbours) {
        if(m_viscousConstant <= 0) return {};

        const auto xi = this->particles().position()[i];
        const auto mu = m_viscousConstant;
        const auto vi = this->particles().velocity()[i];
        const auto m = m_mass;
        const auto di = m_density[i];

        if(di == 0) return {};

        glm::vec2 f{};
        for(auto j : neighbours){
            if(j == i) continue;
            auto xj = this->particles().position()[j];
            auto vj = this->particles().velocity()[j];
            auto dj =  m_density[j];

            f += (dj == 0) ? glm::vec2{0} : (m/dj) * (vj - vi) * ddW(xi - xj);
        }
        return mu * (m/di) * f;
    }

    void integrate(const size_t N, float dt) {
        auto position = this->particles().position();
        auto velocity = this->particles().velocity();
        for(auto i = 0; i < N; i++){
            auto& v = velocity[i];
            auto& p = position[i];
            auto a = m_forces[i]/m_mass;
            v += a * dt;
            p += v * dt;
        }
    }

    void smoothingRadius(float h) {
        m_smoothingRadius = h;
        W = m_kernel(h * 2);
        dW = m_kernel.gradient(h * 2);
        ddW = m_kernel.laplacian(h * 2);
    }

    [[nodiscard]]
    float smoothingRadius() const {
        return m_smoothingRadius;
    }

    [[nodiscard]]
    float gasConstant() const {
        return m_gasConstant;
    }

    void gasConstant(float k) {
        m_gasConstant = k;
    }

    void viscousConstant(float mu) {
        m_viscousConstant = mu;
    }


    [[nodiscard]]
    float viscousConstant() const {
        return m_viscousConstant;
    }

    [[nodiscard]]
    float gravity() const {
        return m_gravity;
    }

    void gravity(float g) {
        m_gravity = g;
        m_gravityForce.y = -g;
    }


private:
    float m_smoothingRadius{1};
    float m_gasConstant{1};
    float m_gravity{9.8};
    float m_mass{1};
    float m_radius{1};
    float m_viscousConstant;
    size_t m_numIterations{1};

    glm::vec2 m_gravityForce{0};

    std::vector<float> m_density;
    std::vector<glm::vec2> m_forces;

    Kernel2D m_kernel;
    std::function<float(glm::vec2)> W;
    std::function<glm::vec2(glm::vec2)> dW;
    std::function<float(glm::vec2)> ddW;
    float h4{0};

    UnBoundedSpacialHashGrid2D m_grid;
};
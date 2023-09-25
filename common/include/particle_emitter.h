#pragma once

#include "particle.h"
#include <memory>

template<template<typename> typename Layout>
class ParticleEmitter {
public:
    ParticleEmitter() = default;

    virtual ~ParticleEmitter() = default;

    void update(float deltaTime);

    virtual void onUpdate(float currentTime, float deltaTime) = 0;

    [[nodiscard]]
    bool enabled() const {
        return m_enabled;
    }

    void enable() {
        m_enabled = true;
    }

    void disable() {
        m_enabled = false;
    }

    void set(std::shared_ptr<Particle2D<Layout>> particles) {
        m_particles = particles;
    }

    virtual void clear() {
        m_currentTime = 0;
    }

protected:
    auto target() {
        return m_particles;
    }


    std::shared_ptr<Particle2D<Layout>> m_particles;
    bool m_enabled{true};
    float m_currentTime{0};
};

template<template<typename> typename Layout>
void ParticleEmitter<Layout>::update(float deltaTime) {
    if(!enabled()) return;
    onUpdate(m_currentTime, deltaTime);
    m_currentTime += deltaTime;
}

template<template<typename> typename Layout>
using Emitters = std::vector<std::unique_ptr<ParticleEmitter<Layout>>>;
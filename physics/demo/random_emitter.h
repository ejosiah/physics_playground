#pragma once

#include "model2d.h"
#include "particle_emitter.h"
#include <memory>

template<template<typename> typename Layout>
class RandomParticleEmitter : public ParticleEmitter<Layout>{
public:
    RandomParticleEmitter() = default;

    ~RandomParticleEmitter() override = default;

    void onUpdate(float currentTime, float deltaTime) override;

    RandomParticleEmitter& radius(float value) {
        m_radius = value;
        return *this;
    }

    RandomParticleEmitter& bounds(Bounds2D bounds) {
        m_bounds = bounds;
        return *this;
    }

    RandomParticleEmitter& numParticles(int numParticles) {
        m_numParticles = numParticles;
        return *this;
    }

    RandomParticleEmitter& restitution(float value) {
        m_restitution = value;
        return *this;
    }

    [[nodiscard]]
    int numParticles() const {
        return m_numParticles;
    }

private:
    float m_radius{};
    Bounds2D m_bounds{};
    int m_numParticles{};
    float m_restitution{};
};
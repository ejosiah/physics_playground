#pragma once

#include "particle_emitter.h"
#include <glm/glm.hpp>
#include <random>
#include <limits>

template<template<typename> typename Layout>
class PointParticleEmitter2D : public ParticleEmitter<Layout> {
public:
    class Builder;

    PointParticleEmitter2D(
            const glm::vec2& origin
            , const glm::vec2& direction
            , float speed
            , float spreadAngleDeg
            , int maxNumOfNewParticlePerSecond = 1
            , int maxNumOfParticles = std::numeric_limits<int>::max()
            , uint32_t seed = 0
            , ProtoTypeParticle2D prototype = {});

    static Builder builder();

    ~PointParticleEmitter2D() override = default;

    void onUpdate(float currentTime, float deltaTime) override;

    void emit(size_t maxNewNumberOfParticles, float deltaTime);

    void add(glm::vec2 position, glm::vec2 velocity);

private:
    float random();

public:
    int maxNumberOfParticlePerSecond;
    int maxNumberOfParticles;

private:
    std::default_random_engine m_rng;
    float m_firstFrameTimeInSeconds{};
    int m_numberOfEmittedParticles{};

    glm::vec2 m_origin;
    glm::vec2 m_direction;
    float m_speed;
    float m_spreadAngleRad;
    ProtoTypeParticle2D m_prototype;
};


template<template<typename> typename Layout>
class PointParticleEmitter2D<Layout>::Builder final {
public:
    Builder& withOrigin(glm::vec2 origin) {
        m_origin = origin;
        return *this;
    }

    Builder& withDirection(const glm::vec2& direction) {
        m_direction = direction;
        return *this;
    }

    Builder& withSpeed(float speed) {
        m_speed = speed;
        return *this;
    }

    Builder& withSpreadAngleInDegrees(float spreadAngle) {
        m_spreadAngleInDegrees = spreadAngle;
        return *this;
    }

    Builder& withMaxNumberOfNewParticlesPerSecond(int value) {
        m_maxNumberOfNewParticlesPerSecond = value;
        return *this;
    }

    Builder& withMaxNumberOfParticles(int value) {
        m_maxNumberOfParticles = value;
        return *this;
    }

    Builder& withRandomSeed(uint32_t seed) {
        m_seed = seed;
        return *this;
    }
    Builder& withRadius(float radius) {
        m_prototype.radius = radius;
        return *this;
    }

    Builder& withMass(float mass) {
        m_prototype.inverseMass = mass <= 0 ? 0 : 1.f/mass;
        return *this;
    }

    Builder& withRestitution(float restitution){
        m_prototype.restitution = restitution;
        return *this;
    }

    PointParticleEmitter2D<Layout> build() const {
        return PointParticleEmitter2D<Layout>{
                m_origin,
                m_direction,
                m_speed,
                m_spreadAngleInDegrees,
                m_maxNumberOfNewParticlesPerSecond,
                m_maxNumberOfParticles,
                m_seed,
                m_prototype
        };
    }

    std::shared_ptr<PointParticleEmitter2D<Layout>> makeShared() const {
        return std::make_shared<PointParticleEmitter2D<Layout>>(
                    m_origin,
                    m_direction,
                    m_speed,
                    m_spreadAngleInDegrees,
                    m_maxNumberOfNewParticlesPerSecond,
                    m_maxNumberOfParticles,
                    m_seed,
                    m_prototype
                );
    }

    std::unique_ptr<PointParticleEmitter2D<Layout>> makeUnique() const {
        return std::make_unique<PointParticleEmitter2D<Layout>>(
                    m_origin,
                    m_direction,
                    m_speed,
                    m_spreadAngleInDegrees,
                    m_maxNumberOfNewParticlesPerSecond,
                    m_maxNumberOfParticles,
                    m_seed,
                    m_prototype
                );
    }

private:
    int m_maxNumberOfNewParticlesPerSecond = 1;
    int m_maxNumberOfParticles = std::numeric_limits<int>::max();
    glm::vec2 m_origin{0, 0};
    glm::vec2 m_direction{0, 1};
    float m_speed = 1.0;
    float m_spreadAngleInDegrees = 90.0;
    uint32_t m_seed = 0;
    ProtoTypeParticle2D m_prototype{};
};

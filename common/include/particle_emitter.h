#pragma once

#include "particle.h"
#include "emitter/emitter.h"
#include <memory>

template<template<typename> typename Layout>
struct ParticlePointConsumer {

    void set(std::shared_ptr<Particle2D<Layout>> particles) {
        m_particles = particles;
    }

    void set(const ProtoTypeParticle2D& prototype = {}){
        m_prototype = prototype;
    }

    operator bool () const {
        return static_cast<bool>(m_particles);
    }

    void use(const glm::vec2& position, const glm::vec2& velocity = glm::vec2{0}) {
        m_particles->add(position, velocity, m_prototype.inverseMass, m_prototype.radius, m_prototype.restitution);
    }

    float offset(float deltaTime) {
        return m_prototype.radius * 1.25f;
    }

    std::shared_ptr<Particle2D<Layout>> m_particles;
    ProtoTypeParticle2D m_prototype;
};


template<template<typename> typename Layout>
class ParticleEmitter {
public:
    ParticleEmitter() = default;

    virtual ~ParticleEmitter() = default;

    void update(float deltaTime);

    virtual void onUpdate(float currentTime, float deltaTime) = 0;

    [[nodiscard]]
    bool enabled() const {
        return (m_emitter && m_emitter->enabled()) ||  m_enabled;
    }

    void enable() {
        if(m_emitter){
            m_emitter->enable();
            return;
        }
        m_enabled = true;
    }

    void disable() {
        if(m_emitter){
            m_emitter->disable();
            return;
        }
        m_enabled = false;
    }

    void set(std::shared_ptr<Particle2D<Layout>> particles) {
        m_consumer.set(particles);
        if(m_emitter) {
            m_emitter->set(m_consumer);
        }
    }

    virtual void clear() {
        if(m_emitter){
            m_emitter->clear();
            return;
        }
        m_currentTime = 0;
    }

protected:
    auto target() {
        return m_consumer.m_particles;
    }


    bool m_enabled{true};
    float m_currentTime{0};

    std::unique_ptr<Emitter<2, ParticlePointConsumer<Layout>>> m_emitter;
    ParticlePointConsumer<Layout> m_consumer{};
};

template<template<typename> typename Layout>
void ParticleEmitter<Layout>::update(float deltaTime) {
    if(m_emitter){
        m_emitter->update(deltaTime);
        return;
    }
    if(!enabled()) return;
    onUpdate(m_currentTime, deltaTime);
    m_currentTime += deltaTime;
}

template<template<typename> typename Layout>
using Emitters = std::vector<std::unique_ptr<ParticleEmitter<Layout>>>;
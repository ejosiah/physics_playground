#pragma once

#include <glm/glm.hpp>

template<glm::length_t L, typename Consumer>
class Emitter{
public:
    Emitter() = default;

    virtual ~Emitter() = default;

    void update(float deltaTime) {
        if(!enabled()) return;
        onUpdate(m_currentTime, deltaTime);
        m_currentTime += deltaTime;
    }

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

    void set(Consumer consumer){
        m_consumer = consumer;
    }

    virtual void clear() {
        m_currentTime = 0;
    }

protected:
    Consumer m_consumer;
    bool m_enabled{true};
    float m_currentTime{};
};
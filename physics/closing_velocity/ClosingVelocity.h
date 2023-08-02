#pragma once
#include "application_2d.h"

class ClosingVelocity : public Application2D {
public:
    ClosingVelocity() : Application2D("Closing Velocity") {}

    ~ClosingVelocity() override = default;

    void createScene() override {
        const auto entity = m_registry.create();
        m_registry.emplace<Circle>(entity, 0.3f);
        m_registry.emplace<Position>(entity, 0.f, 0.f);
        m_registry.emplace<Color>(entity, 1.0f, 1.0f, 0.0f, 1.0f);
        m_registry.emplace<Layer>(entity, 0u);
    }
};
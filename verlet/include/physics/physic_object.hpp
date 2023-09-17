#pragma once
#include "collision_grid.hpp"
#include <glm/glm.hpp>


struct PhysicObject
{
    // Verlet
    glm::vec2 position      = {0.0f, 0.0f};
    glm::vec2 last_position = {0.0f, 0.0f};
    glm::vec2 acceleration  = {0.0f, 0.0f};
    glm::vec3 color{0};

    PhysicObject() = default;

    explicit
    PhysicObject(glm::vec2 position_)
        : position(position_)
        , last_position(position_)
    {}

    void setPosition(glm::vec2 pos)
    {
        position      = pos;
        last_position = pos;
    }

    void update(float dt)
    {
        const glm::vec2 last_update_move = position - last_position;
        const glm::vec2 new_position = position + last_update_move + (acceleration - last_update_move * 40.0f) * (dt * dt);
        last_position           = position;
        position                = new_position;
        acceleration = {0.0f, 0.0f};
    }

    void stop()
    {
        last_position = position;
    }

    void slowdown(float ratio)
    {
        last_position = last_position + ratio * (position - last_position);
    }

    [[nodiscard]]
    float getSpeed() const
    {
        return glm::distance(position, last_position);
    }

    [[nodiscard]]
    glm::vec2 getVelocity() const
    {
        return position - last_position;
    }

    void addVelocity(glm::vec2 v)
    {
        last_position -= v;
    }

    void setPositionSameSpeed(glm::vec2 new_position)
    {
        const glm::vec2 to_last = last_position - position;
        position           = new_position;
        last_position      = position + to_last;
    }

    void move(glm::vec2 v)
    {
        position += v;
    }
};

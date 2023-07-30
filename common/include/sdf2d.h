#pragma once

#include <glm/glm.hpp>

inline float sdBox( glm::vec2& position, glm::vec2 box )
{
    glm::vec2 d = glm::abs(position)-box;
    return glm::length(glm::max(d,0.f)) + glm::min(glm::max(d.x,d.y),0.f);
}
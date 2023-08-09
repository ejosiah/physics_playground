#pragma once

#include <glm/glm.hpp>
#include "model.h"

template<glm::length_t L>
inline float sdf(glm::vec2& position, const Bounds<L>& bounds) {
    const auto dim = dimensions(bounds);
    return sdf(dim, position);
}


inline float sdBox(const glm::vec2& box, glm::vec2& position){
    glm::vec2 d = glm::abs(position)-box;
    return glm::length(glm::max(d,0.f)) + glm::min(glm::max(d.x,d.y),0.f);
}

inline float sdfCircle(const glm::vec2& position, float cRadius, const glm::vec2& cCenter = glm::vec2(0)) {
    return glm::length(position - cCenter) - cRadius;
}

inline float sdfPlane(const glm::vec2& position, float plane){
    return position.y - plane;
}
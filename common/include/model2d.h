#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tuple>

struct Particles2D {
    std::vector<glm::vec2> pPositions;
    std::vector<glm::vec2> cPositions;
    std::vector<glm::vec2> velocities;
    std::vector<float> inverseMass;
    std::vector<float> restitution;
    std::vector<float> radius;
};

using StructOfArraysLayout = Particles2D;

using Bounds2D = std::tuple<glm::vec2, glm::vec2>;


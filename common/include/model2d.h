#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <tuple>

struct Particle2D {
    glm::vec2 pPosition{};
    glm::vec2 cPosition{};
    glm::vec2 velocity{};
    float inverseMass{};
    float restitution{1};
    float radius{1};
    int padding0;
};

struct Particles2D {
    std::vector<glm::vec2> pPositions;
    std::vector<glm::vec2> cPositions;
    std::vector<glm::vec2> velocities;
    std::vector<float> inverseMass;
    std::vector<float> restitution;
    std::vector<float> radius;
};

using ArrayOfStructsLayout = std::vector<Particle2D>;
using StructOfArraysLayout = Particles2D;

using Bounds2D = std::tuple<glm::vec2, glm::vec2>;


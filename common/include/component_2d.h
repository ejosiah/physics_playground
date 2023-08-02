#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <vector>

struct Color {
    float r, g, b, a;
};
struct Position {
    float x, y;
};
struct Velocity {
    float x, y;
};

struct Particle{};

struct Circle{ float radius{}; };
struct Line{ float length{}; };
struct Box{ float width{}; float height{}; };
struct Instance{ uint32_t id{}; };
struct Layer{ uint32_t value{}; };
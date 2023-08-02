#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <vector>

struct Color : public glm::vec4 {};
struct Position : public glm::vec2 {};
struct Velocity : public glm::vec2 {};

struct Particle{};

struct Circle{ float radius{}; };
struct Line{ float length{}; };
struct Box{ float width{}; float height{}; };
struct Instance{ uint32_t id{}; };
struct Layer{ uint32_t value{}; };
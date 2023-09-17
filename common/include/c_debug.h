#pragma once

#include <vector>

static std::vector<int> boundCollisions{};
static std::vector<int> ballCollisions{};

inline void initDebug() {
    boundCollisions.clear();
    ballCollisions.clear();
}
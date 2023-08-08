#pragma once

#include "model2d.h"
#include <vector>
#include <functional>

class PointGenerator2D {
public:
    using Callback = std::function<bool(const glm::vec2&)>;

    PointGenerator2D() = default;

    virtual ~PointGenerator2D() = default;

    [[nodiscard]]
    std::vector<glm::vec2> generate(const Bounds2D& bounds, float spacing) const;

    virtual void forEachPoint(const Bounds2D& bounds, float spacing, Callback callback) const = 0;
};
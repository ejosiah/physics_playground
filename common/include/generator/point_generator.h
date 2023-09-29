#pragma once

#include "model2d.h"
#include <vector>
#include <functional>
#include <glm/glm.hpp>

template<glm::length_t L>
class PointGenerator {
public:
    using Callback = std::function<bool(const glm::vec2&)>;

    PointGenerator() = default;

    virtual ~PointGenerator() = default;

    [[nodiscard]]
    std::vector<glm::vec2> generate(const Bounds<L>& bounds, float spacing) const {
        std::vector<glm::vec2> points;

        forEachPoint(bounds, spacing, [&points](const glm::vec2& point){
            points.push_back(point);
            return true;
        });

        return points;
    }

    virtual void forEachPoint(const Bounds<L>& bounds, float spacing, Callback callback) const = 0;
};

using PointGenerator2D = PointGenerator<2>;
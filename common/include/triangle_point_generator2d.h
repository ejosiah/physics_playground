#pragma once

#include "point_generator2d.h"

class TrianglePointGenerator final : public PointGenerator2D {
public:
    void forEachPoint(const Bounds2D &bounds, float spacing, Callback callback) const final;
};
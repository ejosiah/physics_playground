#include "triangle_point_generator2d.h"
#include <cmath>

void TrianglePointGenerator::forEachPoint(const Bounds2D &bounds, float spacing, Callback callback) const {
    const auto halfSpacing = spacing / 2.0f;
    const auto ySpacing = spacing * std::sqrtf(3.0f) / 2.0f;
    const auto [boxWidth, boxHeight] = dimensions(bounds);

    glm::vec2 position{};
    bool hasOffset = false;
    bool shouldQuit = false;
    for (int j = 0; j * ySpacing <= boxHeight && !shouldQuit; ++j) {
        position.y = j * ySpacing + bounds.lower.y;

        auto offset = (hasOffset) ? halfSpacing : 0.0f;

        for (int i = 0; i * spacing + offset <= boxWidth && !shouldQuit; ++i) {
            position.x = i * spacing + offset + bounds.lower.x;
            if (!callback(position)) {
                shouldQuit = true;
                break;
            }
        }

        hasOffset = !hasOffset;
    }
}
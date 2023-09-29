#include "generator/grid_point_generator2d.h"

void GridPointGenerator2D::forEachPoint(const Bounds2D &bounds, float spacing, Callback callback) const {
    const auto [boxWidth, boxHeight] = dimensions(bounds);

    bool shouldQuit = false;
    glm::vec2 position;
    for (int j = 0; j * spacing <= boxHeight && !shouldQuit; ++j) {
        position.y = j * spacing + bounds.lower.y;

        for (int i = 0; i * spacing <= boxWidth && !shouldQuit; ++i) {
            position.x = i * spacing + bounds.lower.x;
            shouldQuit = !callback(position);
            if (shouldQuit) {
                break;
            }
        }
    }
}
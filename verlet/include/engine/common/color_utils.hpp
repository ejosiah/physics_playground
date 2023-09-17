#pragma once
#include "utils.hpp"
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

using sfColor = glm::vec3;

struct ColorUtils
{
    template<typename T>
    static sfColor createColor(T r, T g, T b)
    {
        return { static_cast<float>(r/255.f), static_cast<float>(g/255.f), static_cast<float>(b/255.f) };
    }

    template<typename TVec3>
    static sfColor createColor(TVec3 vec)
    {
        return { static_cast<uint8_t>(vec.x), static_cast<uint8_t>(vec.y), static_cast<uint8_t>(vec.z) };
    }

    static sfColor interpolate(sfColor color_1, sfColor color_2, float ratio)
    {
        return ColorUtils::createColor(
                static_cast<float>(color_1.r) + ratio * static_cast<float>(color_2.r - color_1.r),
                static_cast<float>(color_1.g) + ratio * static_cast<float>(color_2.g - color_1.g),
                static_cast<float>(color_1.b) + ratio * static_cast<float>(color_2.b - color_1.b)
        );
    }

    static sfColor getRainbow(float t)
    {
        const float r = sin(t);
        const float g = sin(t + 0.33f * 2.0f * glm::pi<float>());
        const float b = sin(t + 0.66f * 2.0f * glm::pi<float>());
        return createColor(255 * r * r, 255 * g * g, 255 * b * b);
    }

};
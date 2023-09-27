#pragma once

#include "types.h"
#include "model.h"
#include "particle.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


#define _15_over_pi (15.f/glm::pi<float>())
#define _45_over_pi (45.f/glm::pi<float>())
#define _90_over_pi (90.f/glm::pi<float>())
#define _315_over_64_pi (315.f/(64.f * glm::pi<float>()))

template<glm::length_t L>
struct SphParticle {
    using VecType = Particles<L, SeparateFieldMemoryLayout>::VecType;
    static constexpr auto Zero = VecType{};

    std::shared_ptr<Particles<L, SeparateFieldMemoryLayout>> data{};
    std::vector<VecType> pressure;
    std::vector<float> density;
    float smoothingRadius{0.2};
    float gasConstant{20};
    float viscosityConstant{0.99};
    float gravity{0.1};
    float mass{1};

    auto size() const {
        return data->size();
    }
};

template<glm::length_t L>
inline auto safeLength(glm::vec<L, float> R) {
    auto rr = glm::dot(R, R);
    return rr == 0.f ? 0.f : sqrt(rr);
}


template<glm::length_t L>
struct Kernel {

    auto operator()(float h) {
          const auto c = _315_over_64_pi/std::powf(h, 9);
          const auto h2 = h*h;

          return [=](glm::vec<L, float> R) {
              auto r = safeLength(R);
              if(r > h) return 0.f;
              const auto x = (h2 - r*r);
              return c * x * x * x;
          };
    }

    auto gradient(float h) {
        const auto c = _45_over_pi/std::powf(h, 6);

        return [=](glm::vec<L, float> R) {
            auto r = safeLength(R);
            if(r == 0 || r > h + 0.001) return zero;

            const auto x = h - r;
            return -c * x * x * R/r;
        };
    }

    auto  laplacian(float h) {
        const auto h2 = h * h;
        const auto _2h3 = 2 * h2 * h;
        return [=](glm::vec<L, float> R){
            const auto r = safeLength(R);
            if(r == 0 || r > h) return 0.f;
            const auto r2 = r * r;
            const auto r3 = r2 * r;
            return r3/_2h3 + r2/h2 + h/(2*r) - 1;
        };
    }

    static constexpr glm::vec<L, float> zero{};
};


using SphParticle2D = SphParticle<2>;
using SphParticle3D = SphParticle<3>;

using Kernel2D = Kernel<2>;
using Kernel3D = Kernel<3>;
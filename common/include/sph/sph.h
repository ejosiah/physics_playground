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

template<glm::length_t L>
struct SphParticle {
    using VecType = Particles<L, SeparateFieldMemoryLayout>::VecType;
    static constexpr auto Zero = VecType{};

    std::shared_ptr<Particles<L, SeparateFieldMemoryLayout>> data{};
    std::vector<VecType> pressure;
    std::vector<float> density;
    float smoothingRadius{0.2};
    float gasConstant{100};
    float viscosityConstant{0};
    float gravity{0.1};
    float mass{1};

    auto size() const {
        return data->size();
    }
};

template<glm::length_t L>
struct Kernel {

    auto operator()(float h) {
          const auto inv_h3 = h * h * h;

          return [=](glm::vec<L, float> R) {
              auto r = safeLength(R);
              if(r > h) return 0.f;
              const auto _1_rh = (1 - r / h);
              return _15_over_pi * inv_h3 * _1_rh * _1_rh * _1_rh;
          };
    }

    auto gradient(float h) {
        const auto inv_h4 = h * h * h * h;

        return [=](glm::vec<L, float> R) {
            auto r = safeLength(R);
            if(r == 0 || r > h + 0.001) return zero;

            const auto _1_rh = (1 - r / h);
            return -_45_over_pi * inv_h4 * _1_rh * _1_rh * R/r;
        };
    }

    auto  laplacian(float h) {
        const auto inv_h5 = h * h * h * h * h;
        return [=](glm::vec<L, float> R){
            auto r = safeLength(R);
            if(r == 0 || r > h) return 0.f;
            return _90_over_pi * inv_h5 * (1 - r/h);
        };
    }

//private:
    auto safeLength(glm::vec<L, float> R) {
        auto rr = glm::dot(R, R);
        return rr == 0.f ? 0.f : sqrt(rr);
    }
    static constexpr glm::vec<L, float> zero{};
};

using SphParticle2D = SphParticle<2>;
using SphParticle3D = SphParticle<3>;

using Kernel2D = Kernel<2>;
using Kernel3D = Kernel<3>;
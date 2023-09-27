#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
static std::vector<int> boundCollisions{};
static std::vector<int> ballCollisions{};
static std::array<std::array<std::vector<int>, 10>, 2> threadGroup;

//static std::array<std::vector<glm::vec4>, 2> groupColor{{
//
//    {
//        glm::vec4(1, 0, 0, 1),
//        glm::vec4(0, 1, 0, 1),
//        glm::vec4(0, 0, 1, 1),
//        glm::vec4(1, 1, 0, 1)
//     },
//    {
//            glm::vec4(1, 0.7, 0.3, 1),
//            glm::vec4(0.3, 1, 0.6, 1),
//            glm::vec4(0.7, 0.3, 1, 1),
//            glm::vec4(1, 0.2, 0.2, 1)
//    }
//
//}};

static std::array<std::vector<glm::vec4>, 2> groupColor{{

    {
        glm::vec4(0.5),
        glm::vec4(0.5),
        glm::vec4(0.5),
        glm::vec4(0.5),
     },
    {
            glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 1, 0, 1),
            glm::vec4(0, 1, 0, 1),
    }

}};


static int g_numThreads = 0;

inline void initDebug() {
    boundCollisions.clear();
    ballCollisions.clear();
    for(auto& pass : threadGroup){
        for(auto& group : pass){
            group.clear();
        }
    }
}
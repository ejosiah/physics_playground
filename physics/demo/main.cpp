#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>


int main(int, char**){
    World2D<SeparateFieldMemoryLayout> world{"physics world", {glm::vec2(0), glm::vec2(20)}, {1024, 1024}};
    world.run();
}
#include "sdf2d.h"
#include "world2d.h"
#include <fmt/format.h>
#include <iostream>


int main(int, char**){
    World2D<SeparateFieldMemoryLayout> world{"physics world", {150, 150}, {1024, 1024}};
    world.run();
}
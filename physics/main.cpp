#include <iostream>
#include "world2d.h"
#include <fmt/format.h>
#include "sdf2d.h"


int main(int, char**){
    World2D<InterleavedMemoryLayout> world{"physics world", {20, 20}, {1024, 1024}};
    world.run();
}
#include <iostream>
#include "engine/VerletIntegratorApp.hpp"

int main(int, char**){
    VerletIntegrationApp app{};
    app.run();
    std::cout << "verlet integration" << "\n";
}
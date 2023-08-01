find_package(benchmark REQUIRED)

add_executable(profiling main.cpp)
target_link_libraries(profiling common benchmark::benchmark ${CONAN_LIBS})
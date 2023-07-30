cmake_minimum_required(VERSION 3.24)
project(physics_playground)

enable_testing()

set(CMAKE_CXX_STANDARD 20)

find_program(GLSLC glslc REQUIRED)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(${CMAKE_CURRENT_LIST_DIR}/dependencies/vglib.github.io/cmake/utilities.cmake)

find_package(Vulkan)

add_definitions(
        -DGLM_FORCE_RADIANS
        -DGLFW_INCLUDE_VULKAN
        -DGLM_FORCE_SWIZZLE
)

set(GLSL_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/resources/shaders)
set(SPV_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin")
compile_glsl_directory(SRC_DIR "${GLSL_SOURCE_DIR}" OUT_DIR "${SPV_DIR}" INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/data/shaders")

add_subdirectory(dependencies)
add_subdirectory(common)
add_subdirectory(physics)
file(GLOB_RECURSE HPP_FILES include/util/*.hpp)
file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(verlet ${HPP_FILES} ${SRC_FILES})
file(GLOB_RECURSE HPP_FILES include/*.hpp)
file(GLOB_RECURSE SRC_FILES src/*.cpp)


message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(verlet ${HPP_FILES} ${SRC_FILES})
target_link_libraries(verlet common)
target_include_directories(verlet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB_RECURSE TEST_FILES ${CMAKE_CURRENT_LIST_DIR}/test/*.*)
file(GLOB_RECURSE GTEST_LIBS ${CONAN_LIB_DIRS_GTEST}/*.lib)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(COMMON_INCLUDE_DIRS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)


add_library(common ${HPP_FILES} ${CPP_FILES})
target_link_libraries(common VulkanBase ImGuiPlugin Vulkan::Vulkan ${CONAN_LIBS})
target_include_directories(common PUBLIC ${COMMON_INCLUDE_DIRS})

include(GoogleTest)
add_executable(common_test ${HPP_FILES} ${CPP_FILES} ${TEST_FILES})
target_link_libraries(common_test PRIVATE common ${GTEST_LIBS} ${CONAN_LIBS})
gtest_discover_tests(common_test)
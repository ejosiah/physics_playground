add_executable(closing_velocity ClosingVelocity.h main.cpp)
target_link_libraries(closing_velocity VulkanBase ImGuiPlugin Vulkan::Vulkan ${CONAN_LIBS})
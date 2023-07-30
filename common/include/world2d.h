#pragma once

#include "VulkanBaseApp.h"
#include <glm/glm.hpp>
#include "model2d.h"
#include "solver2d.h"
#include <span>
#include <type_traits>
#include "spacial_hash.h"
#include "types.h"
#include "particle.h"
#include "world2d.h"
#include <GraphicsPipelineBuilder.hpp>
#include "DescriptorSetBuilder.hpp"
#include "xforms.h"
#include <VulkanInitializers.h>
#include "profile.h"
#include "ImGuiPlugin.hpp"
#include <memory>

using uDimension = glm::uvec2;
using Dimension = glm::vec2;

struct RenderVertex{
    glm::vec2 position;
    glm::vec4 color;
};

template<template<typename> typename Layout>
class World2D : public VulkanBaseApp {
public:
    World2D(const std::string& title, Dimension simDim, uDimension screenDim);

protected:
    void initApp() final;

    void initCamera();

    void initVertexBuffer();

    void createParticles();

    void fillParticles(int n, int = 0);

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderOverlay(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void solve(float dt);

    void resolveCollision();

    void resolveCollisionGrid();

    void resolveCollision(int ai, int bi, float restitution = 1.f);

    static Dimension computeSimDimensions(float simWidth, Dimension screenDim);

private:
    static Settings create(Dimension screenDim);

    static std::vector<RenderVertex> circle(const glm::vec4& color);

    void createPipeline();

protected:
    void onSwapChainRecreation() override;

private:
    Dimension m_simDim;
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    Camera m_camera;


    struct {
        Particle2D<Layout> handle;
        std::span<glm::vec4> color{};
        VulkanBuffer cBuffer;
        VulkanBuffer buffer;
        VulkanBuffer radiusBuffer;
        std::vector<glm::vec2> velocity;
        std::vector<float> restitution;
        std::vector<float> inverseMass;
        uint32_t max{100000};
        uint32_t active{0};
    } particles;

    int maxParticles{50};

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } vBuffer;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } m_render;
    int m_numIterations{1};
    std::array<double, 100> execTime{};
    float m_restitution{0.8};
    bool m_gravityOn{true};
    SpacialHashGrid2D grid;
    float m_radius{0.5};
    int physicsFrame{1};
};

template<template<typename> typename Layout>
World2D<Layout>::World2D(const std::string &title, Dimension simDim, uDimension screenDim)
        : VulkanBaseApp(title, create(screenDim))
        , m_simDim(simDim)
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
    addPlugin(plugin);
}

template<template<typename> typename Layout>
void World2D<Layout>::initApp() {
    initCamera();
    createParticles();
    initVertexBuffer();
    createDescriptorPool();
    createCommandPool();

    createDescriptorSetLayout();
    updateDescriptorSet();

    createPipeline();
}

template<template<typename> typename Layout>
void World2D<Layout>::initCamera() {
    glm::vec2 simDim = m_simDim;
    auto ar = swapChain.aspectRatio();
    m_camera.model = glm::mat4(1);
    m_camera.proj = vkn::ortho(0., simDim.x, 0, simDim.y);
}

template<template<typename> typename Layout>
VkCommandBuffer *World2D<Layout>::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = m_commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {1, 1, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffer.vertices, &offset);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render.layout, 0, 1, &m_render.descriptorSet, 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, m_render.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &m_camera);
    vkCmdDraw(commandBuffer, vBuffer.vertices.sizeAs<RenderVertex>(), particles.active, 0, 0);

    renderOverlay(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

template<template<typename> typename Layout>
void World2D<Layout>::renderOverlay(VkCommandBuffer commandBuffer) {
    auto& imgui = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
//    auto font = imgui.font("Arial", 15);

    ImGui::Begin("overlay", nullptr, IMGUI_NO_WINDOW);
    ImGui::SetWindowPos({0, 0});
    ImGui::SetWindowSize({ 500, 500 });

    auto avg = std::accumulate(execTime.begin(), execTime.end(), 0.0);
    avg /= (to<double>(execTime.size()));

    ImGui::TextColored({0, 0, 0, 1}, "physics %d ms/frame", to<int>(avg));
    ImGui::TextColored({0, 0, 0, 1}, "%d frames/second", framePerSecond);
    ImGui::TextColored({0, 0, 0, 1}, "%d particles", particles.active);

    ImGui::End();

//    ImGui::Begin("controls");
//    ImGui::SetWindowSize({0, 0});
//
//    if(ImGui::Button("Restart")){
//        auto n = particles.active;
//        fillParticles(0);
//        fillParticles(n);
//    }
//    ImGui::SameLine();
//    if(ImGui::Button("Pause")){
//        paused = !paused;
//    }
//    ImGui::SameLine();
//    ImGui::Checkbox("Gravity", &m_gravityOn);
//
//    ImGui::SliderFloat("Restitution", &m_restitution, 0.01, 1);
//    static int nParticles = 0;
//    static bool dirty = false;
//    dirty |= ImGui::SliderInt("add Particles", &nParticles, 0, maxParticles);
//    if(dirty && !ImGui::IsAnyItemActive()){
//        dirty = false;
//        fillParticles(nParticles, particles.active);
//    }
//    ImGui::End();

    plugin<ImGuiPlugin>(IM_GUI_PLUGIN).draw(commandBuffer);

}

template<template<typename> typename Layout>
void World2D<Layout>::update(float time) {
    static float pTime = 0;

    static int nextPhysicsRun = 0;
    pTime += time;
    nextPhysicsRun++;
    if(nextPhysicsRun%physicsFrame == 0) {
        static int count = 0;
        auto dt = pTime/to<float>(m_numIterations);
        auto duration = profile<chrono::milliseconds>([&] {
            for (auto i = 0; i < m_numIterations; i++) {
                solve(dt);
            }
        });
        execTime[count++] = to<double>(duration.count());
        count %= execTime.size();
        pTime = 0;
    }
}

template<template<typename> typename Layout>
Settings World2D<Layout>::create(Dimension screenDim) {
    Settings settings{};
    settings.width = screenDim.x;
    settings.height = screenDim.y;
    return settings;
}

template<template<typename> typename Layout>
void World2D<Layout>::createCommandPool() {
    m_commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers = m_commandPool.allocateCommandBuffers(swapChainImageCount);
}

template<template<typename> typename Layout>
void World2D<Layout>::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 16> poolSizes{
            {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets},
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets }
            }
    };
    m_descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}

template<template<typename> typename Layout>
void World2D<Layout>::createPipeline() {
    //    @formatter:off

    std::string vertexShader = "shader.vert.spv";

    if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>){
        vertexShader = "shader_sep.vert.spv";
    }

    auto builder = device.graphicsPipelineBuilder();
    m_render.pipeline =
            builder
                    .shaderStage()
                    .vertexShader(vertexShader)
                    .fragmentShader("shader.frag.spv")
                    .vertexInputState()
                    .addVertexBindingDescription(0, sizeof(RenderVertex), VK_VERTEX_INPUT_RATE_VERTEX)
                    .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(RenderVertex, position))
                    .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(RenderVertex, color))
                    .inputAssemblyState()
                    .triangleFan()
                    .viewportState()
                    .viewport()
                    .origin(0, 0)
                    .dimension(swapChain.extent)
                    .minDepth(0)
                    .maxDepth(1)
                    .scissor()
                    .offset(0, 0)
                    .extent(swapChain.extent)
                    .add()
                    .rasterizationState()
                    .cullBackFace()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                    .multisampleState()
                    .rasterizationSamples(settings.msaaSamples)
                    .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                    .colorBlendState()
                    .attachment()
                    .add()
                    .layout()
                    .addPushConstantRange(Camera::pushConstant())
                    .addDescriptorSetLayout(m_render.setLayout)
                    .renderPass(renderPass)
                    .subpass(0)
                    .name("render")
                    .build(m_render.layout);
    //    @formatter:on
}

template<template<typename> typename Layout>
std::vector<RenderVertex> World2D<Layout>::circle(const glm::vec4 &color) {
    std::vector<RenderVertex> vertices{};
    vertices.push_back({ glm::vec2{0}, color });

    auto N = 60;
    auto delta = glm::two_pi<float>()/to<float>(N);
    auto angle = 0.f;
    RenderVertex rv{glm::vec2{0}, color};
    for(auto i = 0; i <= N; i++){
        rv.position =  glm::vec2{ glm::cos(angle),  glm::sin(angle)};
        vertices.push_back(rv);
        angle += delta;
    }

    return vertices;
}

template<template<typename> typename Layout>
void World2D<Layout>::initVertexBuffer() {
    auto vertices = circle(glm::vec4{1, 0, 0, 1});
    vBuffer.vertices = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

template<template<typename> typename Layout>
void World2D<Layout>::onSwapChainRecreation() {
    initCamera();
    createPipeline();
}

template<template<typename> typename Layout>
void World2D<Layout>::createParticles() {
    if constexpr (std::is_same_v<Layout<glm::vec2>, InterleavedMemoryLayout2D>) {
        particles.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                                               InterleavedMemoryLayout2D::Width * particles.max, "particles");
        particles.handle =  createInterleavedMemoryParticle2D(std::span{ reinterpret_cast<InterleavedMemoryLayout2D::Members*>(particles.buffer.map()), particles.max });
    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {
        particles.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec2) * particles.max, "particle_position");
        particles.radiusBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(float) * particles.max, "particle_radius");
        particles.velocity.resize(particles.max);
        particles.inverseMass.resize(particles.max);
        particles.restitution.resize(particles.max);

        particles.handle =
            createSeparateFieldParticle2D(
                    std::span{ as<glm::vec2>(particles.buffer.map()), particles.max }
                    , particles.velocity
                    , particles.inverseMass
                    , particles.restitution
                    , std::span{ as<float>(particles.radiusBuffer.map()), particles.max });
    }
    particles.cBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec4) * particles.max, "particle_color");
    particles.color = std::span{ reinterpret_cast<glm::vec4*>(particles.cBuffer.map()), particles.max };
    fillParticles(50);
}

template<template<typename> typename Layout>
void World2D<Layout>::fillParticles(int N, int offset) {
    particles.active = N + offset;

    // static auto seed = std::random_device{}();
    static auto seed = (1 << 20);

    auto rand = rng(0, 20, seed);
    auto vRand = rng(0, 10, seed + (1 << 10));
    auto cRand = rng(0, 1, seed + (1 << 11));
    for(int i = offset; i < particles.active; i++){
        glm::vec2 position{ rand(), rand()};
        particles.handle.position()[i] = position;
        particles.handle.radius()[i] = m_radius;
        particles.handle.velocity()[i] = {vRand(), vRand()};
        particles.handle.inverseMass()[i] = 1.0;
        particles.color[i] = glm::vec4(cRand(), cRand(), cRand(), 1 );
//        particles.color[i] = glm::vec4(1, 0, 0, 1);
    }
    grid = SpacialHashGrid2D{m_radius * 2, to<int32_t>(particles.active) };
}

template<template<typename> typename Layout>
void World2D<Layout>::createDescriptorSetLayout() {
    if constexpr (std::is_same_v<Layout<glm::vec2>, InterleavedMemoryLayout2D>) {
        m_render.setLayout =
            device.descriptorSetLayoutBuilder()
                .name("particle_set")
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
                .binding(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
            .createLayout();

    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {
        m_render.setLayout =
            device.descriptorSetLayoutBuilder()
                .name("particle_set")
                .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
                .binding(1)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
                .binding(2)
                    .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
            .createLayout();
    }
}

template<template<typename> typename Layout>
void World2D<Layout>::updateDescriptorSet() {
    auto sets = m_descriptorPool.allocate({ m_render.setLayout });
    m_render.descriptorSet = sets[0];
    std::vector<VkWriteDescriptorSet> writes{};
    if constexpr (std::is_same_v<Layout<glm::vec2>, InterleavedMemoryLayout2D>) {
        writes = initializers::writeDescriptorSets<2>();
        writes[0].dstSet = m_render.descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].descriptorCount = 1;
        VkDescriptorBufferInfo particleInfo{particles.buffer, 0, VK_WHOLE_SIZE};
        writes[0].pBufferInfo = &particleInfo;

        writes[1].dstSet = m_render.descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        VkDescriptorBufferInfo colorInfo{particles.cBuffer, 0, VK_WHOLE_SIZE};
        writes[1].pBufferInfo = &colorInfo;
    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {

        writes = initializers::writeDescriptorSets<3>();
        writes[0].dstSet = m_render.descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].descriptorCount = 1;
        VkDescriptorBufferInfo positionInfo{particles.buffer, 0, VK_WHOLE_SIZE};
        writes[0].pBufferInfo = &positionInfo;

        writes[1].dstSet = m_render.descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        VkDescriptorBufferInfo radiusInfo{particles.radiusBuffer, 0, VK_WHOLE_SIZE};
        writes[1].pBufferInfo = &radiusInfo;

        writes[2].dstSet = m_render.descriptorSet;
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].descriptorCount = 1;
        VkDescriptorBufferInfo colorInfo{particles.cBuffer, 0, VK_WHOLE_SIZE};
        writes[2].pBufferInfo = &colorInfo;
    }

    device.updateDescriptorSets(writes);
}

template<template<typename> typename Layout>
Dimension World2D<Layout>::computeSimDimensions(float simWidth, Dimension screenDim) {
    auto ar = screenDim.x/screenDim.y;
    return Dimension{ simWidth } * ar;
}

template<template<typename> typename Layout>
void World2D<Layout>::solve(float dt) {
    const glm::vec2 G = m_gravityOn ? glm::vec2{0, -9.8} : glm::vec2{1, 1};
    const auto N = particles.active;

    auto position = particles.handle.position();
    auto velocity = particles.handle.velocity();
    for(int i = 0; i < N; i++){
        auto& p = position[i];
        position[i] += velocity[i] * dt;
        velocity[i] += G * dt;
    }

    resolveCollisionGrid();

    static auto bounds = std::make_tuple(glm::vec2(0), m_simDim);
    for(int i = 0; i < N; i++){
        boundsCheck(particles.handle, bounds, i);
    }

}

template<template<typename> typename Layout>
void World2D<Layout>::resolveCollision() {
    const auto N = particles.active;
    for(auto i = 0; i < N; i++){
        for(auto j = 0; j < N; j++){
            if(i == j) continue;
            resolveCollision(i, j, m_restitution);
        }
    }
}

template<template<typename> typename Layout>
void World2D<Layout>::resolveCollision(int ia, int ib, float restitution) {
    auto position = particles.handle.position();
    auto velocity = particles.handle.velocity();
    auto radius = particles.handle.radius();
    auto inverseMass = particles.handle.inverseMass();

    auto& pa = position[ia];
    auto& pb = position[ib];
    glm::vec2 dir = pb - pa;
    auto d = glm::length(dir);

    if(d == 0 || d > radius[ia] + radius[ib]) return;

    dir /= d;

    auto corr = (radius[ib] + radius[ia] - d) * .5f;
    pa -= dir * corr;
    pb += dir * corr;

    auto v1 = glm::dot(velocity[ia], dir);
    auto v2 = glm::dot(velocity[ib], dir);

    auto m1 = 1/inverseMass[ia];
    auto m2 = 1/inverseMass[ib];

    auto newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    auto newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);

    velocity[ia] += dir * (newV1 - v1);
    velocity[ib] += dir * (newV2 - v2);
}

template<template<typename> typename Layout>
void World2D<Layout>::resolveCollisionGrid() {
    grid.initialize(particles.handle);

    auto vPositions = particles.handle.position();
    for(int i = 0; i < particles.active; i++){
        auto& position = vPositions[i];
        auto ids = grid.query(position, glm::vec2(m_radius * 2));
        for(int j : ids){
            if(i == j) continue;
            resolveCollision(i, j, m_restitution);
        }
    }

//    for(int i = 0; i < N; i++){
//        for(int j = 0; j < N; j++){
//            if(j == i) continue;
//            resolveCollision(i, j, m_restitution);
//        }
//    }

}

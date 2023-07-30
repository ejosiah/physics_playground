#include "world2d.h"
#include <GraphicsPipelineBuilder.hpp>
#include "DescriptorSetBuilder.hpp"
#include "xforms.h"
#include <VulkanInitializers.h>
#include "profile.h"
#include "ImGuiPlugin.hpp"
#include <memory>
#include <span>

World2D::World2D(const std::string &title, Dimension simDim, uDimension screenDim)
: VulkanBaseApp(title, create(screenDim))
, m_simDim(simDim)
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
    addPlugin(plugin);
}

void World2D::initApp() {
    initCamera();
    createParticles();
    initVertexBuffer();
    createDescriptorPool();
    createCommandPool();

    createDescriptorSetLayout();
    updateDescriptorSet();

    createPipeline();
}

void World2D::initCamera() {
    glm::vec2 simDim = m_simDim;
    auto ar = swapChain.aspectRatio();
    m_camera.model = glm::mat4(1);
    m_camera.proj = vkn::ortho(0., simDim.x, 0, simDim.y);
}


VkCommandBuffer *World2D::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

void World2D::renderOverlay(VkCommandBuffer commandBuffer) {
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

void World2D::update(float time) {
    auto dt = time/to<float>(m_numIterations);

    static int count = 0;
    auto duration = profile<chrono::milliseconds>([&] {
        for (auto i = 0; i < m_numIterations; i++) {
            solve(dt);
        }
    });

    execTime[count++] = to<double>(duration.count());
    count %= execTime.size();
}

Settings World2D::create(Dimension screenDim) {
    Settings settings{};
    settings.width = screenDim.x;
    settings.height = screenDim.y;
    return settings;
}

void World2D::createCommandPool() {
    m_commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers = m_commandPool.allocateCommandBuffers(swapChainImageCount);
}

void World2D::createDescriptorPool() {
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

void World2D::createPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    m_render.pipeline =
        builder
            .shaderStage()
                .vertexShader("shader.vert.spv")
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

std::vector<RenderVertex> World2D::circle(const glm::vec4 &color) {
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

void World2D::initVertexBuffer() {
    auto vertices = circle(glm::vec4{1, 0, 0, 1});
    vBuffer.vertices = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void World2D::onSwapChainRecreation() {
    initCamera();
    createPipeline();
}

void World2D::createParticles() {
    particles.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(Particle2D) * particles.max, "particles");
    particles.cBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec4) * particles.max, "particle_color");

    particles.handle =  std::span{ reinterpret_cast<Particle2D*>(particles.buffer.map()), particles.max };
    particles.color = std::span{ reinterpret_cast<glm::vec4*>(particles.cBuffer.map()), particles.max };
    fillParticles(50);
    particles.color[0] = glm::vec4(0);
}

void World2D::fillParticles(int N, int offset) {
    particles.active = N + offset;

   // static auto seed = std::random_device{}();
    static auto seed = (1 << 20);

    auto rand = rng(0, 20, seed);
    auto vRand = rng(0, 10, seed + (1 << 10));
    auto cRand = rng(0, 1, seed + (1 << 11));
    for(int i = offset; i < particles.active; i++){
        glm::vec2 position{ rand(), rand()};
        particles.handle[i].cPosition = position;
        particles.handle[i].cPosition = position;
        particles.handle[i].radius = m_radius;
        particles.handle[i].velocity = {vRand(), vRand()};
        particles.handle[i].inverseMass = 1.0;
        particles.color[i] = glm::vec4(cRand(), cRand(), cRand(), 1 );
//        particles.color[i] = glm::vec4(1, 0, 0, 1);
    }
    grid = SpacialHashGrid2D{m_radius * 2, to<int32_t>(particles.active) };
}

void World2D::createDescriptorSetLayout() {
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
}

void World2D::updateDescriptorSet() {
    auto sets = m_descriptorPool.allocate({ m_render.setLayout });
    m_render.descriptorSet = sets[0];
    
    
    auto writes = initializers::writeDescriptorSets<2>();
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

    device.updateDescriptorSets(writes);
}

Dimension World2D::computeSimDimensions(float simWidth, Dimension screenDim) {
    auto ar = screenDim.x/screenDim.y;
    return Dimension{ simWidth } * ar;
}

void World2D::solve(float dt) {
    const glm::vec2 G = m_gravityOn ? glm::vec2{0, -9.8} : glm::vec2{1, 1};
    const auto N = particles.active;

    for(int i = 0; i < N; i++){
        auto& p = particles.handle[i];
        p.cPosition += p.velocity * dt;
        p.velocity += G * dt;
//        for(int j = 0; j < N; j++){
//            resolveCollision(particles.handle[j], p, m_restitution);
//        }
//        boundsCheck(p, std::make_tuple(glm::vec2(0), m_simDim));
    }

//    resolveCollision();
    resolveCollisionGrid();

    static auto bounds = std::make_tuple(glm::vec2(0), m_simDim);
    for(int i = 0; i < N; i++){
        boundsCheck(particles.handle[i], bounds);
    }

}

void World2D::resolveCollision() {
    const auto N = particles.active;
    for(auto i = 0; i < N; i++){
        Particle2D& pa = particles.handle[i];
        for(auto j = 0; j < N; j++){
            if(i == j) continue;

            Particle2D& pb = particles.handle[j];
            resolveCollision(pa, pb, m_restitution);
        }
    }
}

void World2D::resolveCollision(Particle2D &pa, Particle2D &pb, float restitution) {
    glm::vec2 dir = pb.cPosition - pa.cPosition;
    auto d = glm::length(dir);

    if(d == 0 || d > pb.radius + pa.radius) return;

    dir /= d;

    auto corr = (pb.radius + pa.radius - d) * .5f;
    pa.cPosition -= dir * corr;
    pb.cPosition += dir * corr;

    auto v1 = glm::dot(pa.velocity, dir);
    auto v2 = glm::dot(pb.velocity, dir);

    auto m1 = 1/pa.inverseMass;
    auto m2 = 1/pb.inverseMass;

    auto newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    auto newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);

    pa.velocity += dir * (newV1 - v1);
    pb.velocity += dir * (newV2 - v2);
}

void World2D::resolveCollisionGrid() {
    grid.initialize(std::span{ particles.handle.data(), particles.active });

    for(int i = 0; i < particles.active; i++){
        auto& thisParticle = particles.handle[i];
        auto ids = grid.query(thisParticle.cPosition, glm::vec2(m_radius * 2));
        for(int j = 0; j < ids.size(); j++ ){
            if(i == j) continue;
            auto& otherParticle = particles.handle[ids[j]];
            resolveCollision(thisParticle, otherParticle, m_restitution);
        }
    }

}

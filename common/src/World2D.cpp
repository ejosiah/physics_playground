#include "world2d.h"
#include "serializer.h"
#include "profile.h"
#include <GraphicsPipelineBuilder.hpp>
#include <DescriptorSetBuilder.hpp>
#include <xforms.h>
#include <VulkanInitializers.h>
#include <ImGuiPlugin.hpp>
#include <yaml-cpp/yaml.h>
#include <memory>

template<template<typename> typename Layout>
World2D<Layout>::World2D(const std::string &title, Bounds2D bounds, uDimension screenDim)
        : VulkanBaseApp(title, create(screenDim))
        , m_bounds(bounds)
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
    addPlugin(plugin);
    auto size = glm::length(glm::distance(bounds.lower, bounds.upper));

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
    const auto [lower, upper] = m_bounds;
    auto ar = swapChain.aspectRatio();
    m_camera.model = glm::mat4(1);
    m_camera.proj = vkn::ortho(lower.x, upper.x, lower.y, upper.y);
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
    vkCmdDraw(commandBuffer, vBuffer.vertices.sizeAs<RenderVertex>(), particles.handle->active, 0, 0);

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

    auto& collisionStats = solver->collisionStats;
    auto cAvg = std::accumulate(collisionStats.average.begin(), collisionStats.average.end(), 0.0);
    cAvg /= collisionStats.average.size();

    ImGui::TextColored({0, 0, 0, 1}, "physics %d ms/frame", to<int>(avg));
    ImGui::TextColored({0, 0, 0, 1}, "%d frames/second", framePerSecond);
    ImGui::TextColored({0, 0, 0, 1}, "%d particles", particles.handle->active);
    ImGui::TextColored({0, 0, 0, 1}, "%d min collisions", collisionStats.min);
    ImGui::TextColored({0, 0, 0, 1}, "%d average collisions", to<int>(cAvg));
    ImGui::TextColored({0, 0, 0, 1}, "%d max collisions", collisionStats.max);

    ImGui::End();



    ImGui::Begin("controls");
    ImGui::Checkbox("debug", &debugMode);
    if(debugMode){
        ImGui::SameLine();
        nextFrame = ImGui::Button("nextFrame");
    }
    if(ImGui::Button("snapshot")){
       snapshot();
    }

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
    ImGui::End();

    plugin<ImGuiPlugin>(IM_GUI_PLUGIN).draw(commandBuffer);

}

template<template<typename> typename Layout>
void World2D<Layout>::update(float time) {
    static float pTime = 0;

    static int nextPhysicsRun = 0;
    pTime += time;
    nextPhysicsRun++;
    auto dt = time;
    if(debugMode){
        if(!nextFrame){
            dt = 0;
        }else{
            nextFrame = false;
        }
    }
    if(nextPhysicsRun%physicsFrame == 0 && dt != 0) {
        static int count = 0;
        emitter.update(dt);
        auto duration = profile<chrono::milliseconds>([&] {
              solver->run(dt);
        });
        execTime[count++] = to<double>(duration.count());
        count %= execTime.size();
        pTime = 0;
    }
    transferStateToGPU();

//    static float newBalls = 0;
////    newBalls += time;
//    if(newBalls > 0.05){
//        newBalls = 0;
//        static auto cRand = rng(0, 1, (1 << 11));
//        if(particles.handle->active < startParticles){
//            for(int i = particles.handle->active; i < particles.handle->active + 1; i++){
//                glm::vec2 position{m_radius, m_bounds.upper.y - m_radius};
//                glm::vec2 velocity{ 27, 0 };
//                particles.handle->position()[i] = position;
//                particles.handle->previousPosition()[i] = position - velocity * 0.016667f;
//                particles.handle->velocity()[i] = velocity;
//                particles.handle->radius()[i] = m_radius;
//                particles.handle->inverseMass()[i] = 1.0;
//                particles.handle->restitution()[i] = m_restitution;
//                particles.color[i] = glm::vec4(cRand(), cRand(), cRand(), 1 );
//            }
//            particles.handle->active += 1;
//            solver->numParticles(particles.handle->active);
//        }
//    }
}
template<template<typename> typename Layout>
void World2D<Layout>::transferStateToGPU() {
    particles.pBuffer.copy(particles.position);
    particles.rBuffer.copy(particles.radius);

    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        device.copy(particles.pBuffer, particles.buffer, particles.pBuffer.size);
        device.copy(particles.rBuffer, particles.radiusBuffer, particles.radiusBuffer.size);
    });
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
        particles.handle =  createInterleavedMemoryParticle2DPtr(std::span{ reinterpret_cast<InterleavedMemoryLayout2D::Members*>(particles.buffer.map()), particles.max });
    }else if constexpr (std::is_same_v<Layout<glm::vec2>, SeparateFieldMemoryLayout2D>) {
        particles.pBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, sizeof(glm::vec2) * particles.max, "cpu_particle_position");
        particles.rBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, sizeof(float) * particles.max, "cpu_particle_radius");

        particles.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(glm::vec2) * particles.max, "particle_position");
        particles.radiusBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(float) * particles.max, "particle_radius");
        particles.position.resize(particles.max);
        particles.prevPosition.resize(particles.max);
        particles.velocity.resize(particles.max);
        particles.inverseMass.resize(particles.max);
        particles.restitution.resize(particles.max);
        particles.radius.resize(particles.max);

        particles.handle =
                createSeparateFieldParticle2DPtr(
                        particles.position
                        , particles.prevPosition
                        , particles.velocity
                        , particles.inverseMass
                        , particles.restitution
                        , particles.radius
                        );
    }
    particles.cBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec4) * particles.max, "particle_color");
    particles.color = std::span{ reinterpret_cast<glm::vec4*>(particles.cBuffer.map()), particles.max };

    emitter =
        RandomParticleEmitter<Layout>{particles.handle}
                .radius(m_radius)
                .bounds(m_bounds)
                .numParticles(startParticles)
                .restitution(m_restitution);
    colorParticles();
//    loadParticles();
    solver = std::make_unique<BasicSolver<Layout>>(particles.handle, m_bounds, m_radius, m_numIterations);


}

template<template<typename> typename Layout>
void World2D<Layout>::colorParticles() {

    auto cRand = rng(0, 1, (1 << 11));

    std::generate(particles.color.data(), particles.color.data() + startParticles, [&](){
        return  glm::vec4(cRand(), cRand(), cRand(), 1 );
    });
    particles.color[0] = glm::vec4(0);
}

template<template<typename> typename Layout>
void World2D<Layout>::loadParticles()  {
    YAML::Node node = YAML::LoadFile("world.yaml");
    YAML::Node nodeParticles = node["particles"];
    YAML::Node nodeFields = nodeParticles["fields"];
    spdlog::info("loaded capacity {}", nodeParticles["capacity"].as<int>());
    spdlog::info("num fields {}", nodeFields.size());

    static auto seed = (1 << 20);

    auto rand = random(m_bounds, seed);
    auto vRand = rng(0, 10, seed + (1 << 10));
    auto cRand = rng(0, 1, seed + (1 << 11));

    particles.handle->active = nodeFields.size();
    for(auto i = 0; i < nodeFields.size(); i++){
        auto fields = nodeFields[i].as<Fields>();

        glm::vec2 position = fields.position;
        glm::vec2 velocity = fields.velocity;
        particles.handle->position()[i] = position;
        particles.handle->previousPosition()[i] = position - velocity * 0.016667f;
        particles.handle->velocity()[i] = velocity;
        particles.handle->radius()[i] = m_radius;
        particles.handle->inverseMass()[i] = 1.0;
        particles.handle->restitution()[i] = m_restitution;
        particles.color[i] = glm::vec4(cRand(), cRand(), cRand(), 1 );
    }

    particles.color[0] = glm::vec4(0);
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
    spdlog::info("updating descriptorSet");
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
        spdlog::info("interleaved writes  created");
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
        spdlog::info("separate fields writes  created");
    }
    device.updateDescriptorSets(writes);
}

template<template<typename> typename Layout>
Dimension World2D<Layout>::computeSimDimensions(float simWidth, Dimension screenDim) {
    auto ar = screenDim.x/screenDim.y;
    return Dimension{ simWidth } * ar;
}

template<template<typename> typename Layout>
void World2D<Layout>::snapshot() {
    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "bounds";
    emitter << YAML::Value;
    emitter << YAML::BeginMap;
        emitter << YAML::Key << "min";
        emitter << YAML::Value << YAML::BeginSeq << m_bounds.lower.x << m_bounds.lower.y << YAML::EndSeq;
        emitter << YAML::Key << "max";
        emitter << YAML::Value << YAML::BeginSeq << m_bounds.upper.x << m_bounds.upper.y << YAML::EndSeq;
    emitter << YAML::EndMap;

    emitter << YAML::Key << "particles";
    emitter << YAML::Value;
    emitter << *particles.handle;

    emitter << YAML::EndMap;

    std::ofstream fout{"world.yaml"};
    if(fout.bad()) return;
    fout << emitter.c_str();
}


template<template<typename> typename Layout>
RandomParticleEmitter<Layout>::RandomParticleEmitter(std::shared_ptr<Particle2D<Layout>> particles)
        :ParticleEmitter<Layout>(particles) {

}

template<template<typename> typename Layout>
void RandomParticleEmitter<Layout>::RandomParticleEmitter::onUpdate(float currentTime, float deltaTime) {
    if(this->enabled()){
        this->disable();
        this->m_particles->active = m_numParticles;

        static auto seed = (1 << 20);

        auto pRand = random(m_bounds, seed);
        auto vRand = rng(0, 10, seed + (1 << 10));
        auto cRand = rng(0, 1, seed + (1 << 11));
        for(int i = 0; i < this->m_particles->active; i++){
            glm::vec2 position = pRand();
            glm::vec2 velocity{ vRand(), vRand() };
            this->m_particles->position()[i] = position;
            this->m_particles->previousPosition()[i] = position - velocity * 0.016667f;
            this->m_particles->velocity()[i] = velocity;
            this->m_particles->radius()[i] = m_radius;
            this->m_particles->inverseMass()[i] = 1.0;
            this->m_particles->restitution()[i] = this->m_restitution;
        }
    }
}


template World2D<InterleavedMemoryLayout>;
template World2D<SeparateFieldMemoryLayout>;


#include "sph/sph_sim.h"

SphSim::SphSim(const std::string &title, Bounds2D bounds, uDimension screenDim, Emitters<SeparateFieldMemoryLayout>&& emitters)
        : VulkanBaseApp(title, create(screenDim))
        , m_bounds(bounds)
        , m_emitters(std::move(emitters))
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
    addPlugin(plugin);
}

void SphSim::initApp() {
    initCamera();
    createParticles();
    initVertexBuffer();
    createDescriptorPool();
    createCommandPool();

    createDescriptorSetLayout();
    updateDescriptorSet();

    createPipeline();
}

void SphSim::initCamera() {
    const auto [lower, upper] = m_bounds;
    m_camera.model = glm::mat4(1);
    m_camera.proj = vkn::ortho(lower.x, upper.x, lower.y, upper.y);
}

void SphSim::createParticles() {
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

    particles.handle.data =
            createSeparateFieldParticle2DPtr(
                    particles.position
                    , particles.prevPosition
                    , particles.velocity
                    , particles.inverseMass
                    , particles.restitution
                    , particles.radius
            );
    particles.cBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec4) * particles.max, "particle_color");
    particles.color = std::span{ reinterpret_cast<glm::vec4*>(particles.cBuffer.map()), particles.max };
    particles.handle.density.resize(particles.max);
    particles.handle.pressure.resize(particles.max);

    std::generate(particles.color.begin(), particles.color.end(), [&](){
        return  glm::vec4(1);
    });


    solver = std::make_unique<SphSolver2D<SeparateFieldMemoryLayout>>(
            Kernel2D{},
            particles.handle.smoothingRadius,
            0.1f,
            particles.handle.gasConstant,
            particles.handle.viscosityConstant,
            particles.handle.gravity,
            particles.handle.mass,
            particles.max,
            particles.handle.data,
            m_bounds,
            1);

    options.h = particles.handle.smoothingRadius;
    options.k = particles.handle.gasConstant/options.h;
    options.g = particles.handle.gravity/options.h;

    for(auto& emitter : m_emitters){
        emitter->set(particles.handle.data);
        emitter->update(0);
    }
    transferStateToGPU();
}

void SphSim::initVertexBuffer() {
    auto vertices = circle(glm::vec4{1, 0, 0, 1});
    vBuffer.vertices = device.createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void SphSim::createDescriptorPool() {
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

void SphSim::createDescriptorSetLayout() {
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

void SphSim::updateDescriptorSet() {
    auto sets = m_descriptorPool.allocate({ m_render.setLayout });
    m_render.descriptorSet = sets[0];

    auto writes = initializers::writeDescriptorSets<3>();
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
    device.updateDescriptorSets(writes);
}

void SphSim::createCommandPool() {
    m_commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers = m_commandPool.allocateCommandBuffers(swapChainImageCount);
}

VkCommandBuffer *SphSim::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = m_commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 0, 1};
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
    vkCmdDraw(commandBuffer, vBuffer.vertices.sizeAs<glm::vec2>(), particles.handle.data->size(), 0, 0);

    renderOverlay(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SphSim::renderOverlay(VkCommandBuffer commandBuffer) {
    auto& imgui = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);

    ImGui::Begin("Smoothed particle hydrodynamics");
    ImGui::SetWindowSize({400, 150});
    options.reset |= ImGui::SliderFloat("smoothing radius", &options.h, 0.1, 1.0);
    options.reset |= ImGui::SliderFloat("gravity", &options.g, 0, 100);
    options.reset |= ImGui::SliderFloat("pressure constant", &options.k, 35, 750);


    if(!options.start) {
        if (ImGui::Button("start")) {
            options.start = true;
        }
    }else {
        if (ImGui::Button("stop")) {
            options.start = false;
        }
    }
    ImGui::Text("fps %d", framePerSecond);
    ImGui::End();
    
    imgui.draw(commandBuffer);

}

void SphSim::update(float time) {
    if(!options.start) return;
   if (fixedUpdatesPerSecond == 0) {
        fixedUpdate(time);
    } else {
        static float deltaTime = 1.0f / to<float>(fixedUpdatesPerSecond);
        static float elapsedTime = 0;
        elapsedTime += time;
        if (elapsedTime > deltaTime) {
            elapsedTime = 0;
            fixedUpdate(deltaTime);
        }
    }
    transferStateToGPU();
}

void SphSim::fixedUpdate(float deltaTime) {
    for(auto& emitter : m_emitters){
        emitter->update(deltaTime);
    }
    solver->solve(deltaTime);
}

void SphSim::transferStateToGPU() {
    particles.pBuffer.copy(particles.position);
    particles.rBuffer.copy(particles.radius);

    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        device.copy(particles.pBuffer, particles.buffer, particles.pBuffer.size);
        device.copy(particles.rBuffer, particles.radiusBuffer, particles.radiusBuffer.size);
    });
}

Settings SphSim::create(Dimension screenDim) {
    Settings settings{};
    settings.width = screenDim.x;
    settings.height = screenDim.y;
    return settings;
}

std::vector<RenderVertex1> SphSim::circle(const glm::vec4 &color) {
    std::vector<RenderVertex1> vertices{};
    vertices.push_back({ glm::vec2{0}, color });

    auto N = 60;
    auto delta = glm::two_pi<float>()/to<float>(N);
    auto angle = 0.f;
    RenderVertex1 rv{glm::vec2{0}, color};
    for(auto i = 0; i <= N; i++){
        rv.position =  glm::vec2{ glm::cos(angle),  glm::sin(angle)};
        vertices.push_back(rv);
        angle += delta;
    }

    return vertices;}

void SphSim::createPipeline() {
    //    @formatter:off

    std::string vertexShader = "shader_sep.vert.spv";


    auto builder = device.graphicsPipelineBuilder();
    m_render.pipeline =
        builder
            .shaderStage()
                .vertexShader(vertexShader)
                .fragmentShader("shader.frag.spv")
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(RenderVertex1), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(RenderVertex1, position))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(RenderVertex1, color))
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

void SphSim::newFrame() {
    if(options.reset && !ImGui::IsAnyItemActive()){
        options.reset = false;
        const auto h = options.h;
        const auto k = options.k * h;
        const auto g = options.g * h;
        solver->smoothingRadius(h);
        solver->gasConstant(k);
        solver->gravity(g);
    }
}

#include "application_2d.h"
#include <GraphicsPipelineBuilder.hpp>
#include <algorithm>
#include <numeric>

Application2D::Application2D(const std::string& title, Bounds2D bounds, float width)
: VulkanBaseApp(title, settings2d(bounds, width))
, m_bounds(std::move(bounds))
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
    addPlugin(plugin);
}

Settings Application2D::settings2d(Bounds2D bounds, float width) {
    Settings s{};
    const auto [w, h] = dimensions(bounds);
    float ar = w/h;
    s.width =  width;
    s.height = width/ar;
    s.depthTest = true;
    s.enabledFeatures.wideLines = true;
    s.msaaSamples = VK_SAMPLE_COUNT_16_BIT;

    return s;
}

void Application2D::initApp() {
    createScene();
    uploadPrimitives();
    createInstanceData();
    initCamera();
    createDescriptorPool();
    createCommandPool();

    createDescriptorSetLayout();
    updateDescriptorSet();
    createPipeline();
}

void Application2D::uploadPrimitives() {
    createCirclePrimitive();
    createLinePrimitive();
    createVectorPrimitive();
    createBoxPrimitive();
}

void Application2D::createCirclePrimitive() {
    std::vector<glm::vec3> circle{};
    circle.emplace_back(0);

    auto N = 60;
    auto delta = glm::two_pi<float>()/to<float>(N);
    auto angle = 0.f;
    glm::vec3 position{};
    for(auto i = 0; i <= N; i++){
        position =  glm::vec3{ glm::cos(angle),  glm::sin(angle), 0};
        circle.push_back(position);
        angle += delta;
    }
    buffer.numCircleVertices = to<int>(circle.size());
    buffer.circle = device.createDeviceLocalBuffer(circle.data(), BYTE_SIZE(circle), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Application2D::createLinePrimitive() {
    std::vector<glm::vec3> line{ {-0.5, 0, 0}, {0.5, 0, 0} };
    buffer.line = device.createDeviceLocalBuffer(line.data(), BYTE_SIZE(line), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

}

void Application2D::createVectorPrimitive() {
    std::vector<glm::vec3> points{
            {0, 0, 0}, {1, 0, 0},
            {1, 0, 0}, {0.9, 0.05, 0},
            {1, 0, 0}, {0.9, -0.05, 0}
    };
    buffer.vector = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    buffer.numVectorPoints = points.size();
}

void Application2D::createBoxPrimitive() {
    std::vector<glm::vec3> box{
            {-.5, -.5, 0}, {.5, -.5, 0},
            { .5,  .5, 0}, {-.5, .5, 0}
    };

    buffer.box = device.createDeviceLocalBuffer(box.data(), BYTE_SIZE(box), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Application2D::createInstanceData() {
    int maxLayer = 1;
    m_registry.view<const Layer>().each([&maxLayer](const auto& layer){
        maxLayer = std::max(maxLayer, layer.value);
    });

    createCircleInstanceData(to<float>(maxLayer));
    createBoxInstanceData(to<float>(maxLayer));
    createLineInstanceData(to<float>(maxLayer));
    createVectorInstanceData(to<float>(maxLayer));
    m_maxLayer = maxLayer;
}


void Application2D::createCircleInstanceData(float maxLayer) {
    VkDeviceSize size = std::max(VkDeviceSize{1}, m_registry.size<Circle>());

    instancesData.circle.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            , VMA_MEMORY_USAGE_CPU_TO_GPU
            , size * sizeof(InstanceData), "circle_instances");

    auto view = m_registry.view<Circle, Position, Color, Layer>();
    std::vector<InstanceData> instances;
    for(auto [entity, circle, position, color, layer] : view.each()){
        m_registry.emplace<Instance>(entity, to<int>(instances.size()));

        float z = remap(to<float>(layer.value), 0, maxLayer, -0.9f, 0.9f);
        glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
        xform = glm::scale(xform, glm::vec3(circle.radius));

        instances.push_back({ xform, color});
    }

    if(!instances.empty()) {
        instancesData.circle.buffer.copy(instances);
    }

    instancesData.circle.data = reinterpret_cast<InstanceData*>(instancesData.circle.buffer.map());
}

void Application2D::createBoxInstanceData(float maxLayer) {
    VkDeviceSize size = std::max(VkDeviceSize{1}, m_registry.size<Box>());
    instancesData.box.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            , VMA_MEMORY_USAGE_CPU_TO_GPU
            , 2000 * sizeof(InstanceData), "box_instances");

    std::vector<InstanceData> instances;
    auto view = m_registry.view<Box, Position, Color, Layer>();

    for(auto [entity, box, position, color, layer] : view.each()){
        m_registry.emplace<Instance>(entity, to<int>(instances.size()));

        float z = remap(to<float>(layer.value), 0, maxLayer, -0.9f, 0.9f);
        glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
        xform = glm::scale(xform, glm::vec3(box.width, box.height, 1));

        instances.push_back({ xform, color});
    }

    if(!instances.empty()) {
        instancesData.box.buffer.copy(instances);
    }
}
void Application2D::createLineInstanceData(float maxLayer) {
    VkDeviceSize size = std::max(VkDeviceSize{1}, m_registry.size<Line>());
    instancesData.line.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            , VMA_MEMORY_USAGE_CPU_TO_GPU
            , 2000 * sizeof(InstanceData), "line_instances");


    std::vector<InstanceData> instances;
    auto view = m_registry.view<Line, Position, Color, Layer>();

    for(auto [entity, line, position, color, layer] : view.each()){
        m_registry.emplace<Instance>(entity, to<int>(instances.size()));

        glm::mat4 xform = lineTransform(position.value, to<float>(layer.value), maxLayer, line.length, line.angle);
        instances.push_back({ xform, color});
    }

    if(!instances.empty()) {
        instancesData.line.buffer.copy(instances);
    }

    instancesData.line.data = reinterpret_cast<InstanceData*>(instancesData.line.buffer.map());

}

void Application2D::createVectorInstanceData(float maxLayer) {
    using namespace vec_ops;
    auto size = std::max(size_t{1},  m_registry.size<Vector>());
    instancesData.vector.buffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            , VMA_MEMORY_USAGE_CPU_TO_GPU
            , size * sizeof(InstanceData), "vector_instances");
    instancesData.vector.data = reinterpret_cast<InstanceData*>(instancesData.vector.buffer.map());


    std::vector<InstanceData> instances;
    auto view = m_registry.view<Vector, Position, Color, Layer>();

    for(auto [entity, vector, position, color, layer] : view.each()){
        m_registry.emplace<Instance>(entity, to<int>(instances.size()));

        float z = remap(to<float>(layer.value), 0, maxLayer, -0.9f, 0.9f);
        glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
        xform = glm::scale(xform, glm::vec3(magnitude(vector)));
        xform = glm::rotate(xform, angle(vector), {0, 0, 1});
        instances.push_back({ xform, color});
    }


    if(!instances.empty()) {
        instancesData.vector.buffer.copy(instances);
    }
}

void Application2D::initCamera() {
    const auto [min, max] = m_bounds;
    camera.projection = vkn::ortho(min.x, max.x, min.y, max.y, -1, 1);
    camera.view = glm::mat4{1};
    camera.buffer = device.createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::mat4) * 2, "camera");
    camera.buffer.copy(glm::value_ptr(camera.view), sizeof(glm::mat4));
    camera.buffer.copy(glm::value_ptr(camera.projection), sizeof(glm::mat4), sizeof(glm::mat4));
}

void Application2D::createCommandPool() {
    m_commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers = m_commandPool.allocateCommandBuffers(swapChainImageCount);
}


void Application2D::createDescriptorPool() {
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

void Application2D::createDescriptorSetLayout() {
    instancesData.setLayout =
        device.descriptorSetLayoutBuilder()
            .name("instance_set")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT)
            .createLayout();
}

void Application2D::updateDescriptorSet() {
    std::vector<VkWriteDescriptorSet> writes{};
    std::vector<VkDescriptorBufferInfo> infos{
            {camera.buffer, 0, VK_WHOLE_SIZE},
            {instancesData.circle.buffer, 0, VK_WHOLE_SIZE},
            {instancesData.box.buffer, 0, VK_WHOLE_SIZE},
            {instancesData.line.buffer, 0, VK_WHOLE_SIZE},
            {instancesData.vector.buffer, 0, VK_WHOLE_SIZE},
    };
    instancesData.circle.descriptorSet = updateInstanceDescriptor(writes, infos[1], infos[0]);
    instancesData.box.descriptorSet = updateInstanceDescriptor(writes, infos[2], infos[0]);
    instancesData.line.descriptorSet = updateInstanceDescriptor(writes, infos[3], infos[0]);
    instancesData.vector.descriptorSet = updateInstanceDescriptor(writes, infos[4], infos[0]);

    device.updateDescriptorSets(writes);
}

VkDescriptorSet Application2D::updateInstanceDescriptor( std::vector<VkWriteDescriptorSet> &writes
                                                        , VkDescriptorBufferInfo& sbufferInfo
                                                        , VkDescriptorBufferInfo& cBufferInfo){

    auto descriptorSet = m_descriptorPool.allocate({  instancesData.setLayout }).front();
    auto write = initializers::writeDescriptorSet(descriptorSet);

    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &sbufferInfo;
    writes.push_back(write);

    write.dstBinding = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &cBufferInfo;
    writes.push_back(write);


    return descriptorSet;
}

void Application2D::createPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    solidRender.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader("application2d.vert.spv")
                .fragmentShader("application2d.frag.spv")
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
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
                .lineWidth(2.0)
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
                .addDescriptorSetLayout(instancesData.setLayout)
            .renderPass(renderPass)
            .subpass(0)
            .name("render_solid")
        .build(solidRender.layout);

    lineRender.pipeline =
        builder
            .basePipeline(solidRender.pipeline)
            .inputAssemblyState()
                .lines()
            .name("render_line")
        .build(lineRender.layout);
}

VkCommandBuffer* Application2D::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
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

    renderScene(commandBuffer);
    uiOverlay();
    plugin(IM_GUI_PLUGIN).draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void Application2D::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}


void Application2D::renderScene(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    uint32_t vertexCount = buffer.numCircleVertices;
    auto instanceCount = m_registry.size<Circle>();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidRender.pipeline);
    if(instanceCount > 0) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidRender.layout, 0, 1, &instancesData.circle.descriptorSet, 0, VK_NULL_HANDLE);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffer.circle, &offset);
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
    }

    vertexCount = 4;
    instanceCount = m_registry.size<Box>();
    if(instanceCount > 0) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidRender.layout, 0, 1, &instancesData.box.descriptorSet, 0, VK_NULL_HANDLE);

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffer.box, &offset);
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
    }

    vertexCount = 2;
    instanceCount = m_registry.size<Line>();
    if(instanceCount > 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineRender.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineRender.layout, 0, 1, &instancesData.line.descriptorSet, 0, VK_NULL_HANDLE);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffer.line, &offset);
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
    }

    vertexCount = buffer.numVectorPoints;
    instanceCount = m_registry.size<Vector>();
    if(instanceCount > 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineRender.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lineRender.layout, 0, 1, &instancesData.vector.descriptorSet, 0, VK_NULL_HANDLE);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffer.vector, &offset);
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
    }

}

void Application2D::boundsCheck(Position &position, Velocity& velocity, float radius) {
    auto [min, max] = m_bounds;


    min += radius;
    max -= radius;
    auto& p = position.value;
    auto& v = velocity.value;
    if(p.x < min.x){
        p.x = min.x;
        v.x *= -1;
    }
    if(p.x > max.x){
        p.x = max.x;
        v.x *= -1;
    }
    if(p.y < min.y){
        p.y = min.y;
        v.y *= -1;
    }
    if(p.y > max.y){
        p.y = max.y;
        v.y *= -1;
    }
}

glm::mat4 Application2D::lineTransform(glm::vec2 position, float layer, float maxLayer, float mag, float angle) {
    float z = remap(layer, 0, maxLayer, -0.9f, 0.9f);
    glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
    xform = glm::scale(xform, glm::vec3(mag));
    xform = glm::rotate(xform, angle, {0, 0, 1});

    return xform;
}

void Application2D::reload() {
    m_reloadRequested = true;

}

void Application2D::newFrame() {
    if(m_reloadRequested){
        doReload();
        m_reloadRequested = false;
    }
}

void Application2D::doReload() {
    std::vector<entt::entity> entities{};
    m_registry.each([&entities](const auto entity){ entities.push_back(entity); });
    m_registry.destroy(entities.begin(), entities.end());
    createScene();
    createInstanceData();
    updateDescriptorSet();
    invalidateSwapChain();
}

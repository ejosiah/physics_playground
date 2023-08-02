#include <GraphicsPipelineBuilder.hpp>
#include "application_2d.h"
#include <algorithm>
#include <numeric>

void Application2D::initApp() {
    createScene();
    uploadPrimitives();
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
    createBoxPrimitive();
    createInstanceData();
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

    buffer.circle = device.createDeviceLocalBuffer(circle.data(), BYTE_SIZE(circle), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Application2D::createLinePrimitive() {
    std::vector<glm::vec3> line{ {-0.5, 0, 0}, {0.5, 0, 0} };
    buffer.line = device.createDeviceLocalBuffer(line.data(), BYTE_SIZE(line), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

}

void Application2D::createBoxPrimitive() {
    std::vector<glm::vec3> box{
            {-.5, -.5, 0}, {.5, -.5, 0},
            { .5,  .5, 0}, {-.5, .5, 0}
    };

    buffer.box = device.createDeviceLocalBuffer(box.data(), BYTE_SIZE(box), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Application2D::createInstanceData() {
    uint32_t maxLayer = 0;
    m_registry.view<const Layer>().each([&maxLayer](const auto& layer){
        maxLayer = std::max(maxLayer, layer.value);
    });

    createCircleInstanceData(to<float>(maxLayer));
}


void Application2D::createCircleInstanceData(float maxLayer) {
    std::vector<InstanceData> instances;
    auto cView = m_registry.view<Circle, Position, Color, Layer>();

    for(auto [entity, circle, position, color, layer] : cView.each()){
        m_registry.emplace<Instance>(entity, to<uint32_t>(instances.size()));

        float z = remap(to<float>(layer.value), maxLayer, 0, -0.9f, 0.9f);
        glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position, z));
        xform = glm::scale(xform, glm::vec3(circle.radius));

        instances.push_back({ xform, color});
    }

    instancesBuffer.circle = device.createCpuVisibleBuffer(instances.data(), BYTE_SIZE(instances), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void Application2D::initCamera() {
    throw "camera not yet initialized";
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
    std::vector<VkDescriptorBufferInfo> bufferInfo{};
    instancesData.circle.descriptorSet = updateInstanceDescriptor(instancesData.circle.buffer, writes, bufferInfo);
    instancesData.line.descriptorSet = updateInstanceDescriptor(instancesData.line.buffer, writes, bufferInfo);
    instancesData.box.descriptorSet = updateInstanceDescriptor(instancesData.box.buffer, writes, bufferInfo);
    instancesData.vector.descriptorSet = updateInstanceDescriptor(instancesData.vector.buffer, writes, bufferInfo);


}

VkDescriptorSet Application2D::updateInstanceDescriptor(VulkanBuffer& buffer
                                                        , std::vector<VkWriteDescriptorSet> &writes
                                                        , std::vector<VkDescriptorBufferInfo>& bufferInfo) {

    auto descriptorSet = m_descriptorPool.allocate({  instancesData.setLayout }).front();
    auto write = initializers::writeDescriptorSet(descriptorSet);

    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    bufferInfo.push_back(VkDescriptorBufferInfo{buffer, 0, VK_WHOLE_SIZE});
    write.pBufferInfo = &bufferInfo[bufferInfo.size() - 1];
    writes.push_back(write);

    write.dstSet = instancesData.circle.descriptorSet;
    write.dstBinding = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bufferInfo.push_back(VkDescriptorBufferInfo{cameraData.buffer, 0, VK_WHOLE_SIZE});
    write.pBufferInfo =  &bufferInfo[bufferInfo.size() - 1];
    writes.push_back(write);

    return descriptorSet;
}

void Application2D::createPipeline() {
    solidRender.pipeline =
        device.graphicsPipelineBuilder()
            .shaderStage()
                .vertexShader("application2d.vert.spv")
                .fragmentShader("application2d.frag.spv")
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0)
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
                .addDescriptorSetLayout(solidRender.setLayout)
            .renderPass(renderPass)
            .subpass(0)
            .name("render_solid")
        .build(solidRender.layout);
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
    uiOverlay(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void Application2D::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}


void Application2D::renderScene(VkCommandBuffer commandBuffer) {

}

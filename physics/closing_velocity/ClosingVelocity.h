#pragma once
#include "VulkanBaseApp.h"

class ClosingVelocity : public VulkanBaseApp {
public:
    ClosingVelocity(): VulkanBaseApp("Closing velocity") {}

    ~ClosingVelocity() override = default;

protected:
    void initApp() override {
        createDescriptorPool();
        createCommandPool();

        createDescriptorSetLayout();
        updateDescriptorSet();

        createPipeline();
    }

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    void createPipeline();

    void onSwapChainDispose() override {
        VulkanBaseApp::onSwapChainDispose();
    }

    void onSwapChainRecreation() override {
        VulkanBaseApp::onSwapChainRecreation();
    }

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override {
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


        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        return &commandBuffer;    }

    void update(float time) override {
        VulkanBaseApp::update(time);
    }

private:
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } m_render;

};

void ClosingVelocity::createCommandPool() {
    m_commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandBuffers = m_commandPool.allocateCommandBuffers(swapChainImageCount);
}


void ClosingVelocity::createDescriptorPool() {
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

void ClosingVelocity::createDescriptorSetLayout() {

}

void ClosingVelocity::updateDescriptorSet() {

}

void ClosingVelocity::createPipeline() {

}

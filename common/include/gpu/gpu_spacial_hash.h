#pragma once

#include <VulkanDevice.h>
#include <VulkanDescriptorSet.h>
#include <VulkanBuffer.h>
#include <PrefixSum.hpp>
#include <ComputePipelins.hpp>
#include <glm/glm.hpp>
#include <span>

class GpuSpacialHash : public ComputePipelines {
public:
    GpuSpacialHash(
            size_t maxNumParticles,
            VulkanDevice& device,
            VulkanDescriptorPool& descriptorPool,
            std::span<glm::vec2> positions);

    void initialize(VkCommandBuffer commandBuffer, int numParticles);

    std::vector<PipelineMetaData> pipelineMetaData() final;

protected:
    VulkanDevice& device() const {
        return *m_device;
    }

    void createBuffers();

    void createDescriptorSetLayout();

    void createDescriptorSets();

private:
    struct {
        float spacing{1};
        int tableSize{0};
        int numParticles{0};
    } constants;

    VulkanDevice* m_device;
    VulkanDescriptorPool* m_descriptorPool;
    PrefixSum m_prefixSum;

    VulkanDescriptorSetLayout m_setLayout;
    VkDescriptorSet m_descriptorSet;
    VulkanBuffer m_counts;
    VulkanBuffer m_cellEntries;
};
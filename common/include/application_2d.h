#pragma once
#include "VulkanBaseApp.h"
#include "component_2d.h"
#include "model2d.h"
#include "types.h"
#include <tuple>

struct InstanceData{
    glm::mat4 transform;
    Color color;
};

class Application2D : public VulkanBaseApp {
public:
    Application2D(const std::string& title, Bounds2D bounds = std::make_tuple(glm::vec2(-1), glm::vec2(1)));

    ~Application2D() override = default;

    virtual void createScene() = 0;

    virtual void uiOverlay(VkCommandBuffer commandBuffer) {}

    void renderScene(VkCommandBuffer commandBuffer);

    auto remap(auto x, auto a, auto b, auto c, auto d){
        return glm::mix(c, d, (x - a)/(b - a));
    }

protected:
    void initApp() override;

    void createInstanceData();

    void createCircleInstanceData(float maxLayer);

    void uploadPrimitives();

    void createCirclePrimitive();

    void createLinePrimitive();

    void createBoxPrimitive();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    VkDescriptorSet updateInstanceDescriptor( VulkanBuffer& buffer
                                              , std::vector<VkWriteDescriptorSet>& writes
                                             , std::vector<VkDescriptorBufferInfo>& bufferInfo);

    void createCommandPool();

    void createPipeline();

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

private:
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } solidRender;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } lineRender;

    struct{
        VulkanBuffer circle;
        VulkanBuffer line;
        VulkanBuffer box;
        VulkanBuffer vector;
        int numCircleVertices{};
    } buffer;

    struct {
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
        } circle;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
        } line;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
        } box;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
        } vector;
        VulkanDescriptorSetLayout setLayout;

    } instancesData;

    struct {
        glm::mat4 view{};
        glm::mat4 projection{};
        VulkanBuffer buffer;
    } camera;
    Bounds2D m_bounds;

    void initCamera();
};
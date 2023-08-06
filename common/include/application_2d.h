#pragma once
#include "VulkanBaseApp.h"
#include "component_2d.h"
#include "model2d.h"
#include "types.h"
#include <ImGuiPlugin.hpp>
#include <tuple>

struct InstanceData{
    glm::mat4 transform;
    Color color;
};

using Gravity = glm::vec2;

class Application2D : public VulkanBaseApp {
public:
    Application2D(const std::string& title, Bounds2D bounds = std::make_tuple(glm::vec2(-1), glm::vec2(1)));

    static Settings settings2d();

    ~Application2D() override = default;

    virtual void createScene() = 0;

    virtual void uiOverlay() {}

    void renderScene(VkCommandBuffer commandBuffer);

    auto remap(auto x, auto a, auto b, auto c, auto d){
        return glm::mix(c, d, (x - a)/(b - a));
    }

protected:
    void initApp() override;

    void initCamera();

    void createInstanceData();

    void createCircleInstanceData(float maxLayer);

    void createBoxInstanceData(float maxLayer);

    void createLineInstanceData(float maxLayer);

    glm::mat4 lineTransform(glm::vec2 position, float layer, float maxLayer, float mag, float angle);

    void createVectorInstanceData(float maxLayer);

    void uploadPrimitives();

    void createCirclePrimitive();

    void createLinePrimitive();

    void createVectorPrimitive();

    void createBoxPrimitive();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    VkDescriptorSet updateInstanceDescriptor( std::vector<VkWriteDescriptorSet>& writes
                                              , VkDescriptorBufferInfo& sbufferInfo
                                              , VkDescriptorBufferInfo& cBufferInfo);

    void createCommandPool();

    void createPipeline();

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void boundsCheck(Position& position, Velocity& velocity, float radius = 0);

protected:
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
        int numVectorPoints{};
    } buffer;

    struct {
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
            InstanceData* data;
        } circle;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
            InstanceData* data;
        } line;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
            InstanceData* data;
        } box;
        struct {
            VulkanBuffer buffer;
            VkDescriptorSet descriptorSet;
            InstanceData* data;
        } vector;
        VulkanDescriptorSetLayout setLayout;

    } instancesData;

    struct {
        glm::mat4 view{};
        glm::mat4 projection{};
        VulkanBuffer buffer;
    } camera;
    Bounds2D m_bounds;

    int m_maxLayer{1};
    Gravity m_gravity{0, -0.098};
};
#pragma once

#include "model2d.h"
#include "spacial_hash.h"
#include "types.h"
#include "particle.h"
#include "profile.h"
#include <VulkanBaseApp.h>
#include <GraphicsPipelineBuilder.hpp>
#include <DescriptorSetBuilder.hpp>
#include <xforms.h>
#include <VulkanInitializers.h>
#include <ImGuiPlugin.hpp>
#include <glm/glm.hpp>
#include <type_traits>
#include <span>
#include <memory>
#include "particle_emitter.h"
#include "sph.h"
#include "sph_solver.h"

struct RenderVertex1{
    glm::vec2 position;
    glm::vec4 color;
};

using uDimension = glm::uvec2;
using Dimension = glm::vec2;

class SphSim : public VulkanBaseApp {
public:
    SphSim(const std::string& title, Bounds2D bounds, uDimension screenDim, Emitters<SeparateFieldMemoryLayout>&& emitters);

    void initApp() final;

    void initCamera();

    void createParticles();

    void initVertexBuffer();

    void initGpuHashGrid();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderParticles(VkCommandBuffer commandBuffer);

    void renderMetaballs(VkCommandBuffer commandBuffer);

    void renderOverlay(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void fixedUpdate(float deltaTime);

    void transferStateToGPU();

protected:
    void newFrame() override;

    void onSwapChainRecreation() override;

private:
    static Settings create(Dimension screenDim);

    static std::vector<RenderVertex1> circle(const glm::vec4& color);

    static std::vector<RenderVertex1> quad(const glm::vec4& color);

    void createPipeline();

private:
    Bounds2D m_bounds;
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    Camera m_camera;

    struct {
        SphParticle2D handle;
        std::span<glm::vec4> color{};
        VulkanBuffer cBuffer;
        VulkanBuffer buffer;
        VulkanBuffer pBuffer;
        VulkanBuffer rBuffer;
        VulkanBuffer radiusBuffer;
        std::vector<glm::vec2> position;
        std::vector<glm::vec2> prevPosition;
        std::vector<glm::vec2> velocity;
        std::vector<float> restitution;
        std::vector<float> inverseMass;
        std::vector<float> radius;
        uint32_t max{100000};
    } particles;

    struct {
        float h{0.2};
        float g{50};
        float k{250};
        bool start{false};
        bool reset{false};
    } options;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } vBuffer;

    struct {
        VulkanBuffer vertices;
    } qBuffer;

    struct {
        VulkanBuffer counts;
        VulkanBuffer countsCopy;
        VulkanBuffer cellEntries;
        VulkanBuffer cellEntriesCopy;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        UnBoundedSpacialHashGrid2D cpuGrid;

        struct {
            glm::mat4 model{1};
            glm::mat4 view{1};
            glm::mat4 projection{1};
            float spacing{};
            int tableSize{};
        } constants;
    } gpuSpacialHash;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } m_render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
    } m_metaballs;

    int fixedUpdatesPerSecond{120};
    std::vector<std::unique_ptr<ParticleEmitter<SeparateFieldMemoryLayout>>> m_emitters;
    std::unique_ptr<SphSolver2D<SeparateFieldMemoryLayout>> solver;

};
#pragma once

#include "model2d.h"
#include "solver2d.h"
#include "spacial_hash.h"
#include "types.h"
#include "particle.h"
#include "world2d.h"
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
using uDimension = glm::uvec2;
using Dimension = glm::vec2;

struct RenderVertex{
    glm::vec2 position;
    glm::vec4 color;
};

template<template<typename> typename Layout>
using Emitters = std::vector<std::unique_ptr<ParticleEmitter<Layout>>>;

template<template<typename> typename Layout>
class World2D : public VulkanBaseApp {
public:
    World2D() = default;

    World2D(const std::string& title, Bounds2D bounds, uDimension screenDim, Emitters<Layout>&& emitters);

protected:
    void initApp() final;

    void initCamera();

    void initVertexBuffer();

    void createParticles();

    void colorParticles();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderOverlay(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void fixedUpdate(float deltaTime);

    void transferStateToGPU();

    void checkAppInputs() override;

    static Dimension computeSimDimensions(float simWidth, Dimension screenDim);

private:
    static Settings create(Dimension screenDim);

    static std::vector<RenderVertex> circle(const glm::vec4& color);

    void createPipeline();

    void snapshot();

protected:
    void onSwapChainRecreation() override;

private:
    Bounds2D m_bounds;
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    Camera m_camera;
    std::unique_ptr<Solver2D<Layout>> solver;


    struct {
        std::shared_ptr<Particle2D<Layout>> handle;
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
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } vBuffer;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } m_render;
    int m_numIterations{16};
    std::array<double, 100> execTime{};


    float m_restitution{0.5};
    bool m_gravityOn{true};
    float m_radius{0.1};
    int physicsFrame{1};
    bool debugMode{false};
    bool nextFrame{false};
    int fixedUpdatesPerSecond{60};
    std::vector<std::unique_ptr<ParticleEmitter<Layout>>> emitters;

};

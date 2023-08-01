#pragma once

#include "VulkanBaseApp.h"
#include <glm/glm.hpp>
#include "model2d.h"
#include "solver2d.h"
#include <span>
#include <type_traits>
#include "spacial_hash.h"
#include "types.h"
#include "particle.h"
#include "world2d.h"
#include <GraphicsPipelineBuilder.hpp>
#include "DescriptorSetBuilder.hpp"
#include "xforms.h"
#include <VulkanInitializers.h>
#include "profile.h"
#include "ImGuiPlugin.hpp"
#include <memory>

using uDimension = glm::uvec2;
using Dimension = glm::vec2;

struct RenderVertex{
    glm::vec2 position;
    glm::vec4 color;
};

template<template<typename> typename Layout>
class World2D : public VulkanBaseApp {
public:
    World2D() = default;

    World2D(const std::string& title, Dimension simDim, uDimension screenDim);

protected:
    void initApp() final;

    void initCamera();

    void initVertexBuffer();

    void createParticles();

    void fillParticles(int n, int = 0);

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderOverlay(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void transferStateToGPU();

    static Dimension computeSimDimensions(float simWidth, Dimension screenDim);

private:
    static Settings create(Dimension screenDim);

    static std::vector<RenderVertex> circle(const glm::vec4& color);

    void createPipeline();

protected:
    void onSwapChainRecreation() override;

private:
    Dimension m_simDim;
    VulkanDescriptorPool m_descriptorPool;
    VulkanCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    Camera m_camera;
    std::unique_ptr<Solver2D<Layout>> solver;


    struct {
        Particle2D<Layout> handle;
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
        uint32_t active{0};
    } particles;

    int startParticles{50};

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
    int m_numIterations{1};
    std::array<double, 100> execTime{};


    float m_restitution{0.8};
    bool m_gravityOn{true};
    SpacialHashGrid2D<> grid;
    float m_radius{0.5};
    int physicsFrame{1};
};
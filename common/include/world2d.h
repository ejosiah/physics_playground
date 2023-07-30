#pragma once

#include "VulkanBaseApp.h"
#include <glm/glm.hpp>
#include "model2d.h"
#include "solver2d.h"
#include <span>
#include <type_traits>
#include "spacial_hash.h"
#include "types.h"

using uDimension = glm::uvec2;
using Dimension = glm::vec2;

struct RenderVertex{
    glm::vec2 position;
    glm::vec4 color;
};

class World2D : public VulkanBaseApp {
public:
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

    void solve(float dt);

    void resolveCollision();

    void resolveCollisionGrid();

    void resolveCollision(Particle2D& pa, Particle2D& pb, float restitution = 1.f);

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


    struct {
        std::span<Particle2D> handle{};
        std::span<glm::vec4> color{};
        VulkanBuffer cBuffer;
        VulkanBuffer buffer;
        uint32_t max{100000};
        uint32_t active{0};
    } particles;

    int maxParticles{50};

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
    Solver<ArrayOfStructsLayout, SolverType::Basic> solver;
    int m_numIterations{1};
    std::array<double, 100> execTime{};
    float m_restitution{0.8};
    bool m_gravityOn{true};
    SpacialHashGrid2D grid;
    float m_radius{0.5};
};
#pragma once

#include <application_2d.h>
#include <physics/physics.hpp>
#include <thread_pool/thread_pool.hpp>
#include <engine/common/color_utils.hpp>
#include <memory>
#include <tuple>

class VerletIntegrationApp : public Application2D {
public:
    VerletIntegrationApp() : Application2D("Verlet-MultiThread", {glm::vec2(0), glm::vec2(300)}, 1080)
    {
        glm::ivec2 world_size{m_bounds.upper.x, m_bounds.upper.y};
        thread_pool = std::make_unique<tp::ThreadPool>(1);
        solver = std::make_unique<PhysicSolver>(world_size, *thread_pool);

    }

    void createScene() override {
        for(uint64_t id = 0; id < solver->objects.size(); id++){
            const auto& object = solver->objects[id];
            Entity entity{ m_registry };
            entity.add<Circle>(0.5f);
            entity.add<Position>(object.position);
            entity.add<Color>(object.color.r, object.color.g, object.color.b, 1.0f);
            entity.add<Layer>(0);
            entities.emplace_back(entity, id);
        }
    }

protected:
    void update(float time) override {
        auto doPhysics = timeStep(time);

        if(doPhysics){
            if(solver->objects.size() < MaxParticles){
                pendingObjects.reserve(20);
                for (uint32_t i{20}; i--;) {
                    const auto id = solver->createObject({2.0f, (10.0f + 1.1f * i)});
                    solver->objects[id].last_position.x -= 0.2f;
                    solver->objects[id].color = ColorUtils::getRainbow(id * 0.0001f);
                    pendingObjects.push_back(id);
                }
                reload();
            }
            solver->update(DeltaTime);
            for(auto [entity, id] : entities){
                auto delta = solver->objects[id].position -  solver->objects[id].last_position;
                m_registry.get<Position>(entity).value += delta;
                glm::vec3 p =  glm::vec3(m_registry.get<Position>(entity).value, 0);
                p.y = 300 - p.y;
                glm::mat4 xform = glm::translate(glm::mat4(1), p);
                instancesData.circle.data[id].transform = xform;
            }
        }
    }

    bool timeStep(float time) {
        static float deltaTime = 1.0f / static_cast<float>(fps_cap);
        static float elapsedTime = 0;
        elapsedTime += time;

        if(elapsedTime > deltaTime){
            elapsedTime = 0;
            return true;
        }
        return false;
    }

private:
    static constexpr size_t MaxParticles = 20;
    static constexpr uint32_t fps_cap = 60;
    static constexpr float DeltaTime = 0.016666666666f;
    std::unique_ptr<PhysicSolver> solver;
    std::unique_ptr<tp::ThreadPool> thread_pool;
    std::vector<uint64_t> pendingObjects;
    std::vector<std::tuple<entt::entity, uint64_t>> entities;
};

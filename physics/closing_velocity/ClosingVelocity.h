#pragma once
#include "application_2d.h"
#include "Entity.hpp"

class ClosingVelocity : public Application2D {
public:
    ClosingVelocity() : Application2D("Closing Velocity") {}

    ~ClosingVelocity() override = default;

    void createScene() override {
        assert(sizeof(Position) == sizeof(glm::vec2));
        spdlog::info("Pos size: {}", sizeof(Position));
        glm::vec2 pa{0};
        glm::vec2 pb{0.2, -0.3};
        glm::vec2 va{0.2};
        glm::vec2 vb{0.2, -0.2};

        Entity circle1{ m_registry };
        circle1.add<Circle>(0.1f);
        circle1.add<Position>(glm::vec2(0));
        circle1.add<Color>(0.0f, 0.0f, 1.0f, 1.0f);
        circle1.add<Velocity>(va);
        circle1.add<Layer>(0);
        circle1.add<PhysicsObject>();

        auto c1Velocity = m_registry.create();
        m_registry.emplace<Vector>(c1Velocity, glm::vec2(0.2));
        m_registry.emplace<Position>(c1Velocity, glm::vec2(0));
        m_registry.emplace<Color>(c1Velocity, 0.0f, 0.0f, 0.0f, 1.0f);
        m_registry.emplace<Layer>(c1Velocity, 4);
        m_registry.emplace<Ref>(c1Velocity, circle1);

        auto circle2 = m_registry.create();
        m_registry.emplace<Circle>(circle2, 0.1f);
        m_registry.emplace<Position>(circle2, glm::vec2(0.2, -0.3));
        m_registry.emplace<Color>(circle2, 32.f/255.f, 70.f/255.f, 140.f/255.f, 1.0f);
        m_registry.emplace<Velocity>(circle2, vb);
        m_registry.emplace<Layer>(circle2, 0);
        m_registry.emplace<PhysicsObject>(circle2);
        m_registry.emplace<Displacement>(circle2, glm::vec2(0));

        auto c2Velocity = m_registry.create();
        m_registry.emplace<Vector>(c2Velocity, glm::vec2(0.2));
        m_registry.emplace<Position>(c2Velocity, glm::vec2(0));
        m_registry.emplace<Color>(c2Velocity, 0.0f, 0.0f, 0.0f, 1.0f);
        m_registry.emplace<Layer>(c2Velocity, 4);
        m_registry.emplace<Ref>(c2Velocity, circle2);

        m_contactNormal = m_registry.create();
        m_registry.emplace<ContactNormal>(m_contactNormal);
        m_registry.emplace<Vector>(m_contactNormal, glm::normalize(pa - pb));
        m_registry.emplace<Position>(m_contactNormal, pb);
        m_registry.emplace<Color>(m_contactNormal, 0.0f, 1.0f, 0.0f, 1.0f);
        m_registry.emplace<Layer>(m_contactNormal, 5);
        m_registry.emplace<Link>(m_contactNormal, circle1, circle2);

        m_relVelocity = m_registry.create();
        m_registry.emplace<Vector>(m_relVelocity, va - vb);
        m_registry.emplace<Position>(m_relVelocity, pb);
        m_registry.emplace<Color>(m_relVelocity, 0.0f, 1.0f, 0.0f, 1.0f);
        m_registry.emplace<Layer>(m_relVelocity, 4);
    }

    float computeSeparatingVelocity(Link link) {
        glm::vec2 pa = m_registry.get<Position>(link.a);
        glm::vec2 pb = m_registry.get<Position>(link.b);
        glm::vec2 va = m_registry.get<Velocity>(link.a);
        glm::vec2 vb = m_registry.get<Velocity>(link.b);
        glm::vec2 cn = glm::normalize(pa - pb);

        return glm::dot(va - vb, cn);
    }

    void updateRelativeVelocity(Link link){
        glm::vec2 pb = m_registry.get<Position>(link.b);
        glm::vec2 va = m_registry.get<Velocity>(link.a);
        glm::vec2 vb = m_registry.get<Velocity>(link.b);

        m_registry.get<Position>(m_relVelocity) = pb;
        m_registry.get<Vector>(m_relVelocity) = va - vb;

        if(m_SeparatingVelocity < 0){
            m_registry.get<Color>(m_relVelocity) = {1, 0, 0, 1};
        }else {
            m_registry.get<Color>(m_relVelocity) = {0, 1, 0, 1.0};
        }
    }

protected:
    void update(float time) override {
        runPhysics(time);
        updateInstance();
        updateContactNormal();
        auto link = m_registry.get<Link>(m_contactNormal);
        m_SeparatingVelocity = computeSeparatingVelocity(link);
        updateRelativeVelocity(link);
    }

    void runPhysics(float dt) {
        auto view = m_registry.view<Circle, Position, Velocity>();

        for(auto [entity, circle, position, velocity] : view.each()){
            position += velocity * dt;
            velocity += m_gravity * dt;
            boundsCheck(position, velocity, circle.radius);
        }
    }

    void updateInstance() {

        m_registry.view<Circle, Instance, Layer, Position>()
            .each([&](const auto entity, auto circle, auto& instance, auto& layer, auto& position){
                float z = remap(to<float>(layer.value), 0, m_maxLayer, -0.9f, 0.9f);
                glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
                xform = glm::scale(xform, glm::vec3(circle.radius));
                instancesData.circle.data[instance.id].transform = xform;
            });

        m_registry.view<Vector, Ref, Position>()
            .each([&](const auto entity, auto& vector, auto ref, auto& position){
                using namespace  vec_ops;
                vector = m_registry.get<Velocity>(ref.id);
                position = m_registry.get<Position>(ref.id);
            });


        m_registry.view<Vector, Position, Color, Instance, Layer>()
            .each([&](const auto entity, auto vector, auto position, auto color, auto instance, auto layer){
                using namespace  vec_ops;

                float z = remap(to<float>(layer.value), 0, m_maxLayer, -0.9f, 0.9f);
                glm::mat4 xform = glm::translate(glm::mat4(1), glm::vec3(position.x, position.y, z));
                xform = glm::scale(xform, glm::vec3(magnitude(vector)));
                xform = glm::rotate(xform, angle(vector), {0, 0, 1});


                instancesData.vector.data[instance.id].transform = xform;
                instancesData.vector.data[instance.id].color = color;
            });

        m_registry.view<Line, Position, Instance, Layer>().each([&](const auto entity, auto line, auto position, auto instance, auto layer){
            glm::mat4 xform = lineTransform(position, layer.value, m_maxLayer, line.length, line.angle);
            instancesData.line.data[instance.id].transform = xform;
        });
    }

    void updateContactNormal() {
//        m_registry.view<ContactNormal, Layer, Link, Vector, Position, Color>()
//                .each([&](const auto entity, auto _, auto& layer, auto& link, auto& vector, auto& position, auto& color){
//                    glm::vec2 pa = m_registry.get<Position>(link.a);
//                    glm::vec2 pb = m_registry.get<Position>(link.b);
//                    position = pb;
//                    vector = glm::normalize(pa - pb);
//                    if(m_SeparatingVelocity < 0){
//                        color = {1, 0, 0, 1};
//                    }else {
//                        color = {0, 1, 0, 1.0};
//                    }
//                });

        auto view = m_registry.view<ContactNormal, Layer, Link, Vector, Position, Color>();
        for(const auto e : view) {
            auto link = view.get<Link>(e);
            auto& vector = view.get<Vector>(e);
            auto& position = view.get<Position>(e);
            auto& color = view.get<Color>(e);

            glm::vec2 pa = m_registry.get<Position>(link.a);
            glm::vec2 pb = m_registry.get<Position>(link.b);
            position = pb;
            vector = glm::normalize(pa - pb);
            if(m_SeparatingVelocity < 0){
                color = {1, 0, 0, 1};
            }else {
                color = {0, 1, 0, 1.0};
            }
        }
    }

public:
    void uiOverlay() final {
        ImGui::Begin("report", nullptr, IMGUI_NO_WINDOW);
        ImGui::SetWindowPos({0, 0});
        ImGui::SetWindowSize({0, 0});
        if(m_SeparatingVelocity < 0) {
            ImGui::TextColored({1, 0, 0, 1}, "separating velocity %.5f", m_SeparatingVelocity);
        }else {
            ImGui::TextColored({0.14, 0.25, 0.12, 1}, "separating velocity %.5f", m_SeparatingVelocity);
        }
        ImGui::End();
    }

private:
    float m_timeScale;
    entt::entity m_contactNormal;
    entt::entity m_relVelocity;
    float m_SeparatingVelocity{0};

};
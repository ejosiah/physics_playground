#include "application_2d.h"
#include "point_generators.h"
#include <map>
#include <memory>

enum class GeneratorType : int { Grid, Triangle };

class PointGeneration : public Application2D {
public:
    PointGeneration()
    : Application2D("Point generation", {-1, 1})
    {
        generators[GeneratorType::Grid] = std::make_unique<GridPointGenerator2D>();
        generators[GeneratorType::Triangle] = std::make_unique<TrianglePointGenerator>();
    }

    void createScene() override {
        generatePoints();
    }

    void generatePoints() {
        const auto spacing = radius * 1.25f * 2.f;
        const auto region = shrink(m_bounds, radius);
        const auto points = generators[selected]->generate(region, spacing);
        spdlog::info("generated {} points", points.size());
        for(const auto& point : points){
            auto entity = m_registry.create();
            m_registry.emplace<Circle>(entity, radius);
            m_registry.emplace<Color>(entity, 1.0f, 0.0f, 0.0f, 1.0f);
            m_registry.emplace<Position>(entity, point);
            m_registry.emplace<Layer>(entity, 0);
        }

    }

    void uiOverlay() override {
        ImGui::Begin("controls");
        ImGui::Text("Generator type:");

        static int gType = to<int>(selected);
        ImGui::RadioButton("Grid", &gType, to<int>(GeneratorType::Grid)); ImGui::SameLine();
        ImGui::RadioButton("Triangle", &gType, to<int>(GeneratorType::Triangle));

        if(gType != to<int>(selected)){
            selected = to<GeneratorType>(gType);
            reload();
        }

        ImGui::End();
    }

private:
    float radius{0.025};
    GeneratorType selected{GeneratorType::Triangle};
    std::map<GeneratorType, std::unique_ptr<PointGenerator2D>> generators{};
};

int main(int, char**){
    PointGeneration pg{};
    pg.run();
    return 0;
}
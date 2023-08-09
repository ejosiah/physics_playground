#include "application_2d.h"
#include <algorithm>

class CollisionTest : public Application2D {
public:
    CollisionTest() : Application2D("Collision Test", {-1, 1})
    {}

    void createScene() override {

    }

protected:
    void update(float time) override {

    }

private:
    float m_spacing{1};
    int pending{};
};

int main(int, char**){
    CollisionTest{}.run();
    return 0;
}
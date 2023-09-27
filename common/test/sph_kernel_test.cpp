#include "sph/sph.h"
#include <gtest/gtest.h>

struct SphKernelFixture : public ::testing::Test {
public:
    static constexpr float h = 0.2;
};



TEST_F(SphKernelFixture, kernel) {
    Kernel<2> k{};
    auto W = k(h);
    glm::vec2 R{0.03, 0.04};
    auto res = W(R);
    ASSERT_NEAR(161.363, res, 0.01);
}

TEST_F(SphKernelFixture, evaluateKernelAtsmoothingRadius) {
    Kernel<2> k{};
    auto W = k(h);
    glm::vec2 R{h, 0};
    auto res = W(R);
    ASSERT_EQ(0, res);
}

TEST_F(SphKernelFixture, evaluateKernelAbovesmoothingRadius) {
    Kernel<2> k{};
    auto W = k(h);
    glm::vec2 R{h + h, 0};
    auto res = W(R);
    ASSERT_EQ(0, res);
}

TEST_F(SphKernelFixture, KernelWithDegenerateRadius) {
    Kernel2D k{};
    auto W = k(h);
    glm::vec2 R{0};
    auto res = W(R);
    ASSERT_NEAR(195.835, res, 0.01);
}

TEST_F(SphKernelFixture, gradient) {
    Kernel<2> k{};
    auto dW = k.gradient(h);
    glm::vec2 R{0.03, 0.04};

    auto expected = 5035.761;

    auto res = dW(R);
    auto s = glm::sign(res);
    ASSERT_TRUE(s.x == -1 || s.y == -1);

    auto actual = s.x * s.y * glm::length(res);
    ASSERT_NEAR(expected, actual, 0.001);
}

TEST_F(SphKernelFixture, evaluateGradientAtsmoothingRadius) {
    Kernel<2> k{};
    auto dW = k.gradient(h);
    glm::vec2 R{0, h};

    auto w = dW(R);
    auto res = glm::dot(w, w);
    ASSERT_EQ(res, 0);
}

TEST_F(SphKernelFixture, evaluateGradientAbovesmoothingRadius) {
    Kernel<2> k{};
    auto dW = k.gradient(h);
    glm::vec2 R{0, h + h};

    auto w = dW(R);
    auto res = glm::dot(w, w);
    ASSERT_EQ(res, 0);
}

TEST_F(SphKernelFixture, gradientWithDegenrateRadius) {
    Kernel2D k{};
    auto dW = k.gradient(h);
    glm::vec2 R{0};
    auto res = dW(R);
    ASSERT_EQ(0, glm::dot(res, res));
}

TEST_F(SphKernelFixture, laplacian) {
    Kernel2D  k{};
    auto ddW = k.laplacian(h);
    glm::vec2 R{0.03, 0.04};
    auto res = ddW(R);
    ASSERT_NEAR(1.0703, res, 0.01);
}

TEST_F(SphKernelFixture, evaluateLaplacianAtsmoothingRadius) {
    Kernel2D  k{};
    auto ddW = k.laplacian(h);
    glm::vec2 R{0, h};
    auto res = ddW(R);
    ASSERT_EQ(1, res);
}

TEST_F(SphKernelFixture, evaluateLaplacianAbovesmoothingRadius) {
    Kernel2D  k{};
    auto ddW = k.laplacian(h);
    glm::vec2 R{h, h};
    auto res = ddW(R);
    ASSERT_EQ(0, res);
}

TEST_F(SphKernelFixture, laplacianWithDegenerateRadius){
    Kernel2D  k{};
    auto ddW = k.laplacian(h);
    glm::vec2 R{0};
    auto res = ddW(R);
    ASSERT_EQ(0, res);
}
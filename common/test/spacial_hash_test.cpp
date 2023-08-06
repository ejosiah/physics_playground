#include "spacial_hash_fixture.h"
#include <fmt/format.h>
#include <fmt/ranges.h>

TEST_F(SpacialHashGrid2DFixture, gridConstruction) {
    constexpr auto maxNumObjects = 20u;
    UnBoundedSpacialHashGrid2D grid{1, {}, maxNumObjects};

    ASSERT_EQ(grid.numSpacing(), 1.f) << "spacing should be 1";
    ASSERT_EQ(grid.size(), 40) << "grid size should be twice of max objects";
    ASSERT_EQ(grid.counts().size(), 41) << "grid counts should be gridSize + 1";
    ASSERT_EQ(grid.entries().size(), 20) << "grid entries size should be same as max objects";
    ASSERT_EQ(grid.queryIds().size(), 20) << "grid query ids size should be same as max objects";

}


TEST_F(SpacialHashGrid2DFixture, gridInitialization) {
    constexpr auto maxNumObjects = 20u;
    UnBoundedSpacialHashGrid2D grid{1, {}, maxNumObjects};

    std::vector<glm::vec2> positions{};
    positions.emplace_back(1.5, 2.5);
    positions.emplace_back(2.5, 0);
    positions.emplace_back(3.5, 2.5);
    positions.emplace_back(2.3, 0.5);
    positions.emplace_back(1.8, 2.3);

    grid.initialize(positions);

    std::vector<uint32_t> expected{2, 3, 1, 4, 0};
    const auto actual = grid.entries();

    for(int i = 0; i < 5; i++){
        ASSERT_EQ(expected[i], actual[i]) << fmt::format("{} should equal {} at index {}\n", expected[i], actual[i], i);
    }
}

TEST_F(SpacialHashGrid2DFixture, boundedGridInitialization) {
    BoundedSpacialHashGrid2D grid{1, {5, 4}};

    std::vector<glm::vec2> positions{};
    positions.emplace_back(1.2, 1.3);
    positions.emplace_back(2.2, 3.5);
    positions.emplace_back(3.3, 1.5);
    positions.emplace_back(2.6, 3.8);
    positions.emplace_back(1.2, 1.8);

    grid.initialize(positions);

    std::vector<uint32_t> expected{4, 0, 3, 1, 2};
    const auto actual = grid.entries();

    for(int i = 0; i < 5; i++){
        ASSERT_EQ(expected[i], actual[i]) << fmt::format("{} should equal {} at index {}\n", expected[i], actual[i], i);
    }
}

TEST_F(SpacialHashGrid2DFixture, findEntriesInOneCell) {
    std::vector<glm::vec2> positions{{2.1, 2.2}, {2.3, 2.4}, {2.4, 2.8}};

    constexpr auto maxNumObjects = 20u;
    UnBoundedSpacialHashGrid2D grid{1, {}, maxNumObjects};
    grid.initialize(positions);
    grid.query({2, 2}, {1, 1});

    auto results = grid.queryResults();

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(),0) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),1) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),2) != results.end());
}

TEST_F(SpacialHashGrid2DFixture, findEntriesInOneCellWithNagativePositions) {
    std::vector<glm::vec2> positions{{-2.1, -2.2}, {-2.3, -2.4}, {-2.4, -2.8}};

    constexpr auto maxNumObjects = 20u;
    UnBoundedSpacialHashGrid2D grid{1, {}, maxNumObjects};
    grid.initialize(positions);
    grid.query({-2, -2}, {1, 1});

    auto results = grid.queryResults();
    auto iter = positions.begin();

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(),0) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),1) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),2) != results.end());
}

TEST_F(SpacialHashGrid2DFixture, findEntries1CellApart) {
    std::vector<glm::vec2> positions{
            { 1.5, 2.5}, { 2.5,   0},
            { 3.5, 2.5}, { 2.3, 0.5},
            { 1.8, 2.3},  {2.5, 2.5}
    };

    constexpr auto maxNumObjects = 20u;
    UnBoundedSpacialHashGrid2D grid{1, {}, maxNumObjects};
    grid.initialize(positions);
    grid.query({2, 2}, {1, 1});

    auto results = grid.queryResults();
    ASSERT_EQ(results.size(), 4);
    ASSERT_TRUE(std::find(results.begin(), results.end(),0) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),4) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),2) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(),5) != results.end());
}


TEST_F(SpacialHashGrid2DFixture, findEntries1CellApartInBoundedGrid) {
    std::vector<glm::vec2> positions{
            {2.1, 1.2}, {0, 0}, {4, 0},
            {1.2, 2.8}, {4, 2}, {1.8, 2.8},
    };

    BoundedSpacialHashGrid2D grid{1, {5, 4}};
    grid.initialize(positions);
    auto results = grid.query({2, 1}, {1, 1});
    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(), 0) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 3) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 5) != results.end());
}

TEST_F(SpacialHashGrid2DFixture, DISABLED_findEntries1CellApartWithNegativePositionsInBoundedGrid) {
    FAIL() << "Not yet implemented!";
}

TEST_F(SpacialHashGrid2DFixture, findEntriesFromLiveExample) {
    std::vector<glm::vec2> positions {
            {16.1669006, 4.61024904}, { 5.16748476, 2.78193212 },
            {13.4405041, 1.63059223}, { 14.021657, 1.13235247},
    };
//    constexpr auto maxNumObjects = 100000u;
    const auto resolution = 20.f;
    const auto maxRadius = 2.0f;
    constexpr auto maxNumObjects = 100000u;
    UnBoundedSpacialHashGrid2D  grid{maxRadius, glm::vec2(resolution), maxNumObjects};
    grid.initialize(positions);
    auto results = grid.query(positions[2], glm::vec2(maxRadius));
    ASSERT_EQ(results.size(), maxRadius);
    ASSERT_TRUE(std::find(results.begin(), results.end(), 2) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 3) != results.end());
}
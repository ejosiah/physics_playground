#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "spacial_hash.h"

class SpacialHashGrid2DFixture : public ::testing::Test {
protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }
};
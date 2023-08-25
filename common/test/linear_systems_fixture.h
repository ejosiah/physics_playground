#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "big_matrix.h"
#include "sparse_matrix.h"
#include "linear_systems.h"

class LinearSystemsFixture : public ::testing::Test {
protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }
};
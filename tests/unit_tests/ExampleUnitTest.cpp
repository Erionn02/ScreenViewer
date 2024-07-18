#include <gtest/gtest.h>

struct ExampleUnitTest : public testing::Test {
};

TEST_F(ExampleUnitTest, test) {
    ASSERT_TRUE(true);
}


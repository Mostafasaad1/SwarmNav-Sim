/**
 * @file test_obstacle_tracker.cpp
 * @brief Unit tests for ObstacleTrackerNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <memory>

class ObstacleTrackerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
  }

  void TearDown() override
  {
    rclcpp::shutdown();
  }
};

/**
 * Test node construction
 */
TEST_F(ObstacleTrackerTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_obstacle_tracker");
  ASSERT_NE(node, nullptr);
}

/**
 * Test parameter declaration
 */
TEST_F(ObstacleTrackerTest, ParameterDeclaration)
{
  auto node = std::make_shared<rclcpp::Node>("test_obstacle_tracker");

  node->declare_parameter("publish_rate", 10.0);
  node->declare_parameter("obstacle_timeout", 2.0);

  EXPECT_DOUBLE_EQ(node->get_parameter("publish_rate").as_double(), 10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("obstacle_timeout").as_double(), 2.0);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

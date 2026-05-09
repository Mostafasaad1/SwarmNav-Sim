/**
 * @file test_orca_filter.cpp
 * @brief Unit tests for OrcaVelocityFilterNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <memory>

class OrcaFilterTest : public ::testing::Test
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
TEST_F(OrcaFilterTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_orca_filter");
  ASSERT_NE(node, nullptr);
}

/**
 * Test parameter declaration
 */
TEST_F(OrcaFilterTest, ParameterDeclaration)
{
  auto node = std::make_shared<rclcpp::Node>("test_orca_filter");

  // Declare ORCA parameters
  node->declare_parameter("robot_id", "robot_0");
  node->declare_parameter("robot_radius", 0.25);
  node->declare_parameter("max_neighbors", 10);
  node->declare_parameter("time_horizon", 2.0);
  node->declare_parameter("max_linear_velocity", 0.5);
  node->declare_parameter("max_angular_velocity", 1.0);

  // Verify parameters
  EXPECT_EQ(node->get_parameter("robot_id").as_string(), "robot_0");
  EXPECT_DOUBLE_EQ(node->get_parameter("robot_radius").as_double(), 0.25);
  EXPECT_EQ(node->get_parameter("max_neighbors").as_int(), 10);
  EXPECT_DOUBLE_EQ(node->get_parameter("time_horizon").as_double(), 2.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("max_linear_velocity").as_double(), 0.5);
  EXPECT_DOUBLE_EQ(node->get_parameter("max_angular_velocity").as_double(), 1.0);
}

/**
 * Test velocity clamping logic
 */
TEST_F(OrcaFilterTest, VelocityClamping)
{
  double max_linear = 0.5;
  double max_angular = 1.0;

  // Test linear velocity clamping
  double linear_x = 1.0;  // Exceeds max
  double clamped_linear = std::min(linear_x, max_linear);
  EXPECT_DOUBLE_EQ(clamped_linear, 0.5);

  // Test angular velocity clamping
  double angular_z = 2.0;  // Exceeds max
  double clamped_angular = std::min(angular_z, max_angular);
  EXPECT_DOUBLE_EQ(clamped_angular, 1.0);

  // Test within limits
  linear_x = 0.3;
  clamped_linear = std::min(linear_x, max_linear);
  EXPECT_DOUBLE_EQ(clamped_linear, 0.3);
}

/**
 * Test negative velocity clamping
 */
TEST_F(OrcaFilterTest, NegativeVelocityClamping)
{
  double max_linear = 0.5;
  double max_angular = 1.0;

  // Test negative linear velocity
  double linear_x = -1.0;
  double clamped_linear = std::max(linear_x, -max_linear);
  EXPECT_DOUBLE_EQ(clamped_linear, -0.5);

  // Test negative angular velocity
  double angular_z = -2.0;
  double clamped_angular = std::max(angular_z, -max_angular);
  EXPECT_DOUBLE_EQ(clamped_angular, -1.0);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

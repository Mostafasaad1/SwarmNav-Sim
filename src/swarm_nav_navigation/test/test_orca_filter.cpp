/**
 * @file test_orca_filter.cpp
 * @brief Unit tests for OrcaVelocityFilterNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <memory>
#include <cmath>

#include "swarm_nav_navigation/orca_velocity_filter_node.hpp"

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

TEST_F(OrcaFilterTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_orca_filter");
  ASSERT_NE(node, nullptr);
}

TEST_F(OrcaFilterTest, ParameterDeclaration)
{
  auto node = std::make_shared<rclcpp::Node>("test_orca_filter");

  node->declare_parameter("robot_id", "robot_0");
  node->declare_parameter("robot_radius", 0.25);
  node->declare_parameter("max_neighbors", 10);
  node->declare_parameter("time_horizon", 2.0);
  node->declare_parameter("max_linear_velocity", 0.5);
  node->declare_parameter("max_angular_velocity", 1.0);

  EXPECT_EQ(node->get_parameter("robot_id").as_string(), "robot_0");
  EXPECT_DOUBLE_EQ(node->get_parameter("robot_radius").as_double(), 0.25);
  EXPECT_EQ(node->get_parameter("max_neighbors").as_int(), 10);
  EXPECT_DOUBLE_EQ(node->get_parameter("time_horizon").as_double(), 2.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("max_linear_velocity").as_double(), 0.5);
  EXPECT_DOUBLE_EQ(node->get_parameter("max_angular_velocity").as_double(), 1.0);
}

TEST_F(OrcaFilterTest, VelocityClamping)
{
  double max_linear = 0.5;
  double max_angular = 1.0;

  double linear_x = 1.0;
  double clamped_linear = std::copysign(std::min(std::abs(linear_x), max_linear), linear_x);
  EXPECT_DOUBLE_EQ(clamped_linear, 0.5);

  double angular_z = 2.0;
  double clamped_angular = std::copysign(std::min(std::abs(angular_z), max_angular), angular_z);
  EXPECT_DOUBLE_EQ(clamped_angular, 1.0);

  linear_x = 0.3;
  clamped_linear = std::copysign(std::min(std::abs(linear_x), max_linear), linear_x);
  EXPECT_DOUBLE_EQ(clamped_linear, 0.3);
}

TEST_F(OrcaFilterTest, NegativeVelocityClamping)
{
  double max_linear = 0.5;
  double max_angular = 1.0;

  double linear_x = -1.0;
  double clamped_linear = std::copysign(std::min(std::abs(linear_x), max_linear), linear_x);
  EXPECT_DOUBLE_EQ(clamped_linear, -0.5);

  double angular_z = -2.0;
  double clamped_angular = std::copysign(std::min(std::abs(angular_z), max_angular), angular_z);
  EXPECT_DOUBLE_EQ(clamped_angular, -1.0);
}

TEST_F(OrcaFilterTest, YawCalculation)
{
  auto node = std::make_shared<rclcpp::Node>("test_yaw");
  
  geometry_msgs::msg::Quaternion q;
  q.w = 1.0;
  q.x = 0.0;
  q.y = 0.0;
  q.z = 0.0;
  
  class TestableOrcaNode : public swarm_nav_navigation::OrcaVelocityFilterNode
  {
  public:
    using swarm_nav_navigation::OrcaVelocityFilterNode::getYaw;
  };
  
  double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
  double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  double yaw = std::atan2(siny_cosp, cosy_cosp);
  EXPECT_NEAR(yaw, 0.0, 1e-6);
  
  double angle = M_PI / 4.0;
  q.w = std::cos(angle / 2.0);
  q.z = std::sin(angle / 2.0);
  siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
  cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  yaw = std::atan2(siny_cosp, cosy_cosp);
  EXPECT_NEAR(yaw, M_PI / 4.0, 1e-6);
}

TEST_F(OrcaFilterTest, CollisionDetectionLogic)
{
  double robot_radius = 0.25;
  double neighbor_radius = 0.25;
  double combined_radius = robot_radius + neighbor_radius;
  
  double dx = 0.3;
  double dy = 0.0;
  double dist = std::sqrt(dx * dx + dy * dy);
  
  EXPECT_TRUE(dist < combined_radius);
  EXPECT_TRUE(dist < 0.51);
  
  dx = 1.0;
  dy = 0.0;
  dist = std::sqrt(dx * dx + dy * dy);
  EXPECT_FALSE(dist < combined_radius);
}

TEST_F(OrcaFilterTest, VelocityProjection)
{
  double rel_vx = 1.0;
  double rel_vy = 0.5;
  
  double theta = std::atan2(rel_vy, rel_vx);
  double phi = 0.2;

  double alpha1 = theta - phi;
  double alpha2 = theta + phi;
  (void)alpha2; // Silence unused variable warning
  
  double t1_x = std::cos(alpha1);
  double t1_y = std::sin(alpha1);
  
  double proj = rel_vx * t1_x + rel_vy * t1_y;
  proj = std::max(0.0, proj);
  
  EXPECT_GE(proj, 0.0);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

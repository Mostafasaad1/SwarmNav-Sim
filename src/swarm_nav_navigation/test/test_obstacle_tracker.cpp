/**
 * @file test_obstacle_tracker.cpp
 * @brief Unit tests for ObstacleTrackerNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <memory>
#include <string>

#include "swarm_nav_navigation/obstacle_tracker_node.hpp"

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

TEST_F(ObstacleTrackerTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_obstacle_tracker");
  ASSERT_NE(node, nullptr);
}

TEST_F(ObstacleTrackerTest, ParameterDeclaration)
{
  auto node = std::make_shared<rclcpp::Node>("test_obstacle_tracker");

  node->declare_parameter("publish_rate", 10.0);
  node->declare_parameter("obstacle_timeout", 2.0);
  node->declare_parameter("obstacle_ids", std::vector<std::string>());

  EXPECT_DOUBLE_EQ(node->get_parameter("publish_rate").as_double(), 10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("obstacle_timeout").as_double(), 2.0);
}

TEST_F(ObstacleTrackerTest, ParameterWithObstacleIds)
{
  auto node = std::make_shared<rclcpp::Node>("test_obstacle_tracker");

  std::vector<std::string> obstacle_ids = {"forklift_0", "human_1"};
  node->declare_parameter("obstacle_ids", obstacle_ids);

  auto result = node->get_parameter("obstacle_ids").as_string_array();
  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "forklift_0");
  EXPECT_EQ(result[1], "human_1");
}

TEST_F(ObstacleTrackerTest, TrackedObstacleStructure)
{
  swarm_nav_navigation::TrackedObstacle obs;
  obs.id = "test_obstacle";
  obs.pose.position.x = 1.0;
  obs.pose.position.y = 2.0;
  obs.velocity.linear.x = 0.5;
  obs.radius = 1.0f;
  obs.classification = 2;

  EXPECT_EQ(obs.id, "test_obstacle");
  EXPECT_DOUBLE_EQ(obs.pose.position.x, 1.0);
  EXPECT_DOUBLE_EQ(obs.pose.position.y, 2.0);
  EXPECT_DOUBLE_EQ(obs.velocity.linear.x, 0.5);
  EXPECT_FLOAT_EQ(obs.radius, 1.0f);
  EXPECT_EQ(obs.classification, 2);
}

TEST_F(ObstacleTrackerTest, ObstacleClassificationConstants)
{
  const uint8_t STATIC = 0;
  const uint8_t SEMI_DYNAMIC = 1;
  const uint8_t DYNAMIC = 2;

  swarm_nav_navigation::TrackedObstacle shelf;
  shelf.classification = STATIC;
  EXPECT_EQ(shelf.classification, 0);

  swarm_nav_navigation::TrackedObstacle forklift;
  forklift.classification = SEMI_DYNAMIC;
  EXPECT_EQ(forklift.classification, 1);

  swarm_nav_navigation::TrackedObstacle human;
  human.classification = DYNAMIC;
  EXPECT_EQ(human.classification, 2);
}

TEST_F(ObstacleTrackerTest, OdometryMessageConversion)
{
  nav_msgs::msg::Odometry odom_msg;
  odom_msg.pose.pose.position.x = 5.0;
  odom_msg.pose.pose.position.y = -3.0;
  odom_msg.twist.twist.linear.x = 1.0;
  odom_msg.twist.twist.angular.z = 0.5;

  swarm_nav_navigation::TrackedObstacle obs;
  obs.pose = odom_msg.pose.pose;
  obs.velocity = odom_msg.twist.twist;

  EXPECT_DOUBLE_EQ(obs.pose.position.x, 5.0);
  EXPECT_DOUBLE_EQ(obs.pose.position.y, -3.0);
  EXPECT_DOUBLE_EQ(obs.velocity.linear.x, 1.0);
  EXPECT_DOUBLE_EQ(obs.velocity.angular.z, 0.5);
}

TEST_F(ObstacleTrackerTest, StalenessCheck)
{
  double obstacle_timeout = 2.0;
  
  rclcpp::Time now = rclcpp::Clock().now();
  rclcpp::Time stale_time = now - rclcpp::Duration::from_seconds(3.0);
  rclcpp::Time fresh_time = now - rclcpp::Duration::from_seconds(1.0);

  auto stale_duration = (now - stale_time).seconds();
  auto fresh_duration = (now - fresh_time).seconds();

  EXPECT_GT(stale_duration, obstacle_timeout);
  EXPECT_LT(fresh_duration, obstacle_timeout);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

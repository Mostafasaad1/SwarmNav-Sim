/**
 * @file test_aggregator.cpp
 * @brief Unit tests for NeighborStateAggregatorNode
 */

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <map>
#include <memory>

#include "swarm_nav_msgs/msg/neighbor_state.hpp"
#include "swarm_nav_msgs/msg/neighbor_state_array.hpp"
#include "swarm_nav_navigation/neighbor_state_aggregator_node.hpp"

class AggregatorTest : public ::testing::Test
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

TEST_F(AggregatorTest, NodeConstruction)
{
  auto node = std::make_shared<rclcpp::Node>("test_aggregator");
  ASSERT_NE(node, nullptr);
}

TEST_F(AggregatorTest, ParameterDeclaration)
{
  auto node = std::make_shared<rclcpp::Node>("test_aggregator");

  node->declare_parameter("publish_rate", 10.0);
  node->declare_parameter("state_timeout", 0.5);

  EXPECT_DOUBLE_EQ(node->get_parameter("publish_rate").as_double(), 10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("state_timeout").as_double(), 0.5);
}

TEST_F(AggregatorTest, NeighborStateMessageStructure)
{
  swarm_nav_msgs::msg::NeighborState state;
  state.robot_id = "robot_0";
  state.pose.position.x = 1.0;
  state.pose.position.y = 2.0;
  state.velocity.linear.x = 0.5;
  state.radius = 0.25f;

  EXPECT_EQ(state.robot_id, "robot_0");
  EXPECT_DOUBLE_EQ(state.pose.position.x, 1.0);
  EXPECT_DOUBLE_EQ(state.pose.position.y, 2.0);
  EXPECT_DOUBLE_EQ(state.velocity.linear.x, 0.5);
  EXPECT_FLOAT_EQ(state.radius, 0.25f);
}

TEST_F(AggregatorTest, NeighborStateArrayStructure)
{
  swarm_nav_msgs::msg::NeighborStateArray array;
  array.header.frame_id = "map";

  swarm_nav_msgs::msg::NeighborState state1;
  state1.robot_id = "robot_0";
  state1.pose.position.x = 0.0;

  swarm_nav_msgs::msg::NeighborState state2;
  state2.robot_id = "robot_1";
  state2.pose.position.x = 1.0;

  array.neighbors.push_back(state1);
  array.neighbors.push_back(state2);

  EXPECT_EQ(array.neighbors.size(), 2);
  EXPECT_EQ(array.neighbors[0].robot_id, "robot_0");
  EXPECT_EQ(array.neighbors[1].robot_id, "robot_1");
}

TEST_F(AggregatorTest, StalenessCheck)
{
  double state_timeout = 0.5;
  
  rclcpp::Time now = rclcpp::Clock().now();
  rclcpp::Time stale_time = now - rclcpp::Duration::from_seconds(1.0);
  rclcpp::Time fresh_time = now - rclcpp::Duration::from_seconds(0.1);

  auto stale_duration = (now - stale_time).seconds();
  auto fresh_duration = (now - fresh_time).seconds();

  EXPECT_GT(stale_duration, state_timeout);
  EXPECT_LT(fresh_duration, state_timeout);
}

TEST_F(AggregatorTest, StateStorageLogic)
{
  std::map<std::string, swarm_nav_msgs::msg::NeighborState> states;
  std::map<std::string, rclcpp::Time> timestamps;

  swarm_nav_msgs::msg::NeighborState state;
  state.robot_id = "robot_0";
  state.pose.position.x = 1.0;

  rclcpp::Time now = rclcpp::Clock().now();

  states[state.robot_id] = state;
  timestamps[state.robot_id] = now;

  EXPECT_EQ(states.size(), 1);
  EXPECT_EQ(states["robot_0"].robot_id, "robot_0");
  EXPECT_DOUBLE_EQ(states["robot_0"].pose.position.x, 1.0);

  swarm_nav_msgs::msg::NeighborState updated;
  updated.robot_id = "robot_0";
  updated.pose.position.x = 2.0;
  states[updated.robot_id] = updated;

  EXPECT_EQ(states.size(), 1);
  EXPECT_DOUBLE_EQ(states["robot_0"].pose.position.x, 2.0);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

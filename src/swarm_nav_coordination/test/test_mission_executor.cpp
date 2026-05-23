// test_mission_executor.cpp
// Unit tests for MissionExecutorNode lifecycle behavior

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "swarm_nav_coordination/mission_executor_node.hpp"

using namespace std::chrono_literals;

class MissionExecutorNodeTest : public ::testing::Test
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

TEST_F(MissionExecutorNodeTest, NodeConstruction)
{
  auto node = std::make_shared<swarm_nav_coordination::MissionExecutorNode>();
  ASSERT_NE(node, nullptr);

  auto state = node->get_current_state();
  ASSERT_EQ(state.id(), lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED);
}

TEST_F(MissionExecutorNodeTest, ConfigureFailsWithEmptyFilename)
{
  // Default parameter is empty string, which should fail configure
  auto node = std::make_shared<swarm_nav_coordination::MissionExecutorNode>();

  auto state = node->configure();
  ASSERT_EQ(state.id(), lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED);
}

TEST_F(MissionExecutorNodeTest, ConfigureFailsWithNonexistentXml)
{
  rclcpp::NodeOptions options;
  options.parameter_overrides({
    rclcpp::Parameter("bt_xml_filename", "/nonexistent/path/mission_tree.xml")
  });
  auto node = std::make_shared<swarm_nav_coordination::MissionExecutorNode>(options);

  auto state = node->configure();
  ASSERT_EQ(state.id(), lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

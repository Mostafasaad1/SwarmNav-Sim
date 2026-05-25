// Copyright 2026 SwarmNav-Sim Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

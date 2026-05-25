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

// frontier_detector_bt.cpp
// BehaviorTree action node to detect frontiers

#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include "swarm_nav_msgs/msg/frontier_array.hpp"

namespace swarm_nav_coordination
{

class FrontierDetectorBT : public BT::SyncActionNode
{
public:
  FrontierDetectorBT(const std::string & name, const BT::NodeConfiguration & config)
  : BT::SyncActionNode(name, config)
  {
    node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");

    // Get robot_id from blackboard, fallback to "robot_0"
    std::string robot_id = "robot_0";
    config.blackboard->get<std::string>("robot_id", robot_id);

    // Subscribe to frontiers topic
    frontier_sub_ = node_->create_subscription<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers",
      rclcpp::QoS(10),
      [this](swarm_nav_msgs::msg::FrontierArray::SharedPtr msg) {
        latest_frontiers_ = msg;
      }
    );
  }

  static BT::PortsList providedPorts()
  {
    return {
      BT::OutputPort<int>("frontier_count", "Number of frontiers detected")
    };
  }

  BT::NodeStatus tick() override
  {
    RCLCPP_INFO(node_->get_logger(), "Checking for frontiers...");

    // Use real frontier count from subscription
    int frontier_count = 0;
    if (latest_frontiers_) {
      frontier_count = latest_frontiers_->frontiers.size();
    }

    setOutput("frontier_count", frontier_count);

    if (frontier_count > 0) {
      RCLCPP_INFO(node_->get_logger(), "Detected %d frontiers", frontier_count);
      return BT::NodeStatus::SUCCESS;
    }

    RCLCPP_WARN(node_->get_logger(), "No frontiers detected");
    return BT::NodeStatus::FAILURE;
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_sub_;
  swarm_nav_msgs::msg::FrontierArray::SharedPtr latest_frontiers_;
};

}  // namespace swarm_nav_coordination

void RegisterFrontierDetectorBT(BT::BehaviorTreeFactory& factory)
{
  factory.registerNodeType<swarm_nav_coordination::FrontierDetectorBT>("FrontierDetectorBT");
}

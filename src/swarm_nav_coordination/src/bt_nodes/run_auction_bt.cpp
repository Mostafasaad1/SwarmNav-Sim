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

// run_auction_bt.cpp
// BehaviorTree action node that participates in a Vickrey auction and,
// on winning, outputs the assigned frontier as a geometry_msgs::PoseStamped
// ready for consumption by the Nav2 NavigateToPose BT action node.

#include <memory>
#include <string>
#include <unordered_map>

#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include <geometry_msgs/msg/pose_stamped.hpp>

#include "swarm_nav_msgs/msg/auction_result.hpp"
#include "swarm_nav_msgs/msg/frontier_array.hpp"

namespace swarm_nav_coordination
{

class RunAuctionBT : public BT::StatefulActionNode
{
public:
  RunAuctionBT(const std::string & name, const BT::NodeConfiguration & config)
  : BT::StatefulActionNode(name, config), received_result_(false)
  {
    node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");

    std::string robot_id = "robot_0";
    config.blackboard->get<std::string>("robot_id", robot_id);
    robot_id_ = robot_id;

    // Cache frontier positions so we can resolve frontier_id → PoseStamped
    frontier_sub_ = node_->create_subscription<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers",
      rclcpp::QoS(10),
      [this](swarm_nav_msgs::msg::FrontierArray::SharedPtr msg) {
        for (const auto & f : msg->frontiers) {
          geometry_msgs::msg::PoseStamped pose;
          pose.header = msg->header;
          pose.pose.position = f.centroid;
          pose.pose.orientation.w = 1.0;
          frontier_poses_[f.frontier_id] = pose;
        }
      });

    // Listen for auction results addressed to this robot
    result_sub_ = node_->create_subscription<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result",
      rclcpp::QoS(10),
      [this](swarm_nav_msgs::msg::AuctionResult::SharedPtr msg) {
        if (msg->winner_id == robot_id_) {
          auto it = frontier_poses_.find(msg->frontier_id);
          if (it != frontier_poses_.end()) {
            assigned_pose_ = it->second;
            received_result_ = true;
            RCLCPP_INFO(
              node_->get_logger(),
              "Won auction – navigating to frontier %s (%.2f, %.2f)",
              msg->frontier_id.c_str(),
              assigned_pose_.pose.position.x,
              assigned_pose_.pose.position.y);
          } else {
            RCLCPP_WARN(
              node_->get_logger(),
              "Won auction for frontier %s but no pose cached yet",
              msg->frontier_id.c_str());
          }
        }
      });
  }

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<int>("frontier_count", "Number of frontiers to auction"),
      BT::OutputPort<geometry_msgs::msg::PoseStamped>(
        "assigned_frontier", "Navigation goal assigned to this robot")
    };
  }

  BT::NodeStatus onStart() override
  {
    int frontier_count = 0;
    getInput("frontier_count", frontier_count);
    RCLCPP_INFO(node_->get_logger(), "Starting auction for %d frontiers", frontier_count);

    received_result_ = false;
    auction_start_time_ = node_->now();
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    if (received_result_) {
      setOutput("assigned_frontier", assigned_pose_);
      RCLCPP_INFO(
        node_->get_logger(), "Auction complete – goal set (%.2f, %.2f)",
        assigned_pose_.pose.position.x, assigned_pose_.pose.position.y);
      return BT::NodeStatus::SUCCESS;
    }

    const auto elapsed = node_->now() - auction_start_time_;
    if (elapsed.seconds() > 5.0) {
      RCLCPP_WARN(node_->get_logger(), "Auction timeout – no frontier assigned");
      return BT::NodeStatus::FAILURE;
    }

    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    received_result_ = false;
    RCLCPP_WARN(node_->get_logger(), "Auction halted");
  }

private:
  rclcpp::Node::SharedPtr node_;
  std::string robot_id_;

  rclcpp::Subscription<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_sub_;

  std::unordered_map<std::string, geometry_msgs::msg::PoseStamped> frontier_poses_;
  geometry_msgs::msg::PoseStamped assigned_pose_;
  rclcpp::Time auction_start_time_;
  bool received_result_;
};

}  // namespace swarm_nav_coordination

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<swarm_nav_coordination::RunAuctionBT>("RunAuctionBT");
}

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

// map_coverage_check.cpp
// BehaviorTree node to check if map coverage is sufficient

#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include <nav_msgs/msg/occupancy_grid.hpp>

namespace swarm_nav_coordination
{

class MapCoverageCheck : public BT::ConditionNode
{
public:
  MapCoverageCheck(const std::string & name, const BT::NodeConfiguration & config)
  : BT::ConditionNode(name, config)
  {
    node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");

    // Subscribe to global map
    map_sub_ = node_->create_subscription<nav_msgs::msg::OccupancyGrid>(
      "/swarm/global_map",
      rclcpp::QoS(10).transient_local(),
      [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        latest_map_ = msg;
      }
    );
  }

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("coverage_threshold", 0.95, "Required coverage percentage (0.0-1.0)")
    };
  }

  BT::NodeStatus tick() override
  {
    if (!latest_map_) {
      return BT::NodeStatus::FAILURE;
    }

    // Get coverage threshold
    double threshold = 0.95;
    getInput("coverage_threshold", threshold);

    // Calculate coverage
    double coverage = calculateCoverage(latest_map_);

    RCLCPP_DEBUG(
      node_->get_logger(), "Map coverage: %.2f%% (threshold: %.2f%%)",
      coverage * 100.0, threshold * 100.0);

    if (coverage >= threshold) {
      RCLCPP_INFO(node_->get_logger(), "Coverage threshold met: %.2f%%", coverage * 100.0);
      return BT::NodeStatus::SUCCESS;
    }

    return BT::NodeStatus::FAILURE;
  }

private:
  double calculateCoverage(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    int total_cells = 0;
    int known_cells = 0;

    for (const auto & cell : map->data) {
      total_cells++;
      if (cell != -1) {  // Not unknown
        known_cells++;
      }
    }

    return total_cells > 0 ? static_cast<double>(known_cells) / total_cells : 0.0;
  }

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  nav_msgs::msg::OccupancyGrid::SharedPtr latest_map_;
};

}  // namespace swarm_nav_coordination

// Register the node with BehaviorTree.CPP
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<swarm_nav_coordination::MapCoverageCheck>("MapCoverageCheck");
}

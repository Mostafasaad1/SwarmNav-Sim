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

    node_->declare_parameter("world_width", -1.0);
    node_->declare_parameter("world_height", -1.0);
    node_->declare_parameter("world_origin_x", 0.0);
    node_->declare_parameter("world_origin_y", 0.0);

    world_width_ = node_->get_parameter("world_width").as_double();
    world_height_ = node_->get_parameter("world_height").as_double();
    world_origin_x_ = node_->get_parameter("world_origin_x").as_double();
    world_origin_y_ = node_->get_parameter("world_origin_y").as_double();

    use_world_bounds_ = (world_width_ > 0 && world_height_ > 0);

    if (use_world_bounds_) {
      RCLCPP_INFO(
        node_->get_logger(),
        "MapCoverageCheck using world bounds: %.1f x %.1f m at (%.1f, %.1f)",
        world_width_, world_height_, world_origin_x_, world_origin_y_);
    } else {
      RCLCPP_INFO(
        node_->get_logger(),
        "MapCoverageCheck using map info bounds (world bounds not configured)");
    }

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
      RCLCPP_DEBUG(node_->get_logger(), "No map received yet, coverage check FAILURE");
      return BT::NodeStatus::FAILURE;
    }

    double threshold = 0.95;
    getInput("coverage_threshold", threshold);

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
    int known_cells = 0;
    int total_map_cells = map->data.size();

    for (const auto & cell : map->data) {
      if (cell != -1) {
        known_cells++;
      }
    }

    if (!use_world_bounds_) {
      return total_map_cells > 0 ? static_cast<double>(known_cells) / total_map_cells : 0.0;
    }

    double resolution = map->info.resolution;
    int64_t total_world_cells = static_cast<int64_t>((world_width_ / resolution) * (world_height_ / resolution));

    RCLCPP_DEBUG(
      node_->get_logger(),
      "Coverage: known=%d, map_total=%d, world_total=%ld (%.1fx%.1fm @ %.3fm/cell)",
      known_cells, total_map_cells, total_world_cells,
      world_width_, world_height_, resolution);

    return total_world_cells > 0 ? static_cast<double>(known_cells) / total_world_cells : 0.0;
  }

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  nav_msgs::msg::OccupancyGrid::SharedPtr latest_map_;

  double world_width_;
  double world_height_;
  double world_origin_x_;
  double world_origin_y_;
  bool use_world_bounds_;
};

}  // namespace swarm_nav_coordination

void RegisterMapCoverageCheck(BT::BehaviorTreeFactory& factory)
{
  factory.registerNodeType<swarm_nav_coordination::MapCoverageCheck>("MapCoverageCheck");
}

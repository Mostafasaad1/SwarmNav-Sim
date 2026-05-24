// neighbor_state_aggregator_node.cpp
// Aggregates individual NeighborState messages into NeighborStateArray

#include <rclcpp/rclcpp.hpp>
#include "swarm_nav_navigation/neighbor_state_aggregator_node.hpp"

#include <string>
#include <map>
#include <mutex>
#include <chrono>

namespace swarm_nav_navigation
{

NeighborStateAggregatorNode::NeighborStateAggregatorNode()
: Node("neighbor_state_aggregator_node")
{
  this->declare_parameter("publish_rate", 10.0);
  this->declare_parameter("state_timeout", 0.5);
  if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

  double publish_rate = this->get_parameter("publish_rate").as_double();
  state_timeout_ = this->get_parameter("state_timeout").as_double();

  RCLCPP_INFO(this->get_logger(), "Neighbor State Aggregator initialized");
  RCLCPP_INFO(this->get_logger(), "Publish rate: %.1f Hz", publish_rate);
  RCLCPP_INFO(this->get_logger(), "State timeout: %.1f seconds", state_timeout_);

  state_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborState>(
    "/swarm/robot_state",
    rclcpp::SensorDataQoS(),
    [this](swarm_nav_msgs::msg::NeighborState::SharedPtr msg) {
      this->stateCallback(msg);
    });

  array_pub_ = this->create_publisher<swarm_nav_msgs::msg::NeighborStateArray>(
    "/swarm/neighbor_states",
    rclcpp::SensorDataQoS()
  );

  timer_ = this->create_wall_timer(
    std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate)),
    [this]() { this->publishArray(); });

  RCLCPP_INFO(this->get_logger(), "Neighbor State Aggregator ready");
}

void NeighborStateAggregatorNode::stateCallback(swarm_nav_msgs::msg::NeighborState::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);

  states_[msg->robot_id] = *msg;
  timestamps_[msg->robot_id] = this->now();

  RCLCPP_DEBUG(this->get_logger(), "Received state from %s", msg->robot_id.c_str());
}

void NeighborStateAggregatorNode::publishArray()
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = this->now();

  for (auto it = states_.begin(); it != states_.end(); ) {
    if ((now - timestamps_[it->first]).seconds() > state_timeout_) {
      RCLCPP_DEBUG(this->get_logger(), "Removing stale state: %s", it->first.c_str());
      timestamps_.erase(it->first);
      it = states_.erase(it);
    } else {
      ++it;
    }
  }

  swarm_nav_msgs::msg::NeighborStateArray array_msg;
  array_msg.header.stamp = now;
  array_msg.header.frame_id = "map";

  for (const auto & [robot_id, state] : states_) {
    array_msg.neighbors.push_back(state);
  }

  array_pub_->publish(array_msg);
  RCLCPP_DEBUG(this->get_logger(), "Published array with %zu states", states_.size());
}

} // namespace swarm_nav_navigation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::NeighborStateAggregatorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

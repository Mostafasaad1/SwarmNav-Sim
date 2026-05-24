// obstacle_tracker_node.cpp
// Tracks dynamic obstacles (NPCs, forklifts) and broadcasts their states

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "swarm_nav_navigation/obstacle_tracker_node.hpp"

namespace swarm_nav_navigation
{

ObstacleTrackerNode::ObstacleTrackerNode()
: Node("obstacle_tracker_node")
{
  this->declare_parameter("publish_rate", 10.0);
  this->declare_parameter("obstacle_timeout", 2.0);
  this->declare_parameter("obstacle_ids", std::vector<std::string>());
  if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

  double publish_rate = this->get_parameter("publish_rate").as_double();
  obstacle_timeout_ = this->get_parameter("obstacle_timeout").as_double();
  std::vector<std::string> obstacle_ids = this->get_parameter("obstacle_ids").as_string_array();

  RCLCPP_INFO(this->get_logger(), "Obstacle Tracker initialized");
  RCLCPP_INFO(this->get_logger(), "Publish rate: %.1f Hz", publish_rate);
  RCLCPP_INFO(this->get_logger(), "Obstacle timeout: %.1f seconds", obstacle_timeout_);

  for (const auto & obs_id : obstacle_ids) {
    RCLCPP_INFO(this->get_logger(), "Tracking obstacle: %s", obs_id.c_str());
  }

  obstacle_pub_ = this->create_publisher<swarm_nav_msgs::msg::ObstacleArray>(
    "/swarm/tracked_obstacles", rclcpp::SensorDataQoS()
  );

  timer_ = this->create_wall_timer(
    std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate)),
    [this]() { this->publishObstacles(); });

  if (obstacle_ids.empty()) {
    RCLCPP_WARN(
      this->get_logger(),
      "No obstacle IDs configured. "
      "Set 'obstacle_ids' parameter to track simulator obstacles. "
      "Using test obstacles for demonstration.");
    RCLCPP_INFO(this->get_logger(), "Initialized with %zu test obstacles", obstacles_.size());
  } else {
    for (const auto & obs_id : obstacle_ids) {
      std::string topic = "/" + obs_id + "/odom";
      RCLCPP_INFO(this->get_logger(), "Subscribing to: %s", topic.c_str());
      
      auto callback = [this, obs_id](nav_msgs::msg::Odometry::SharedPtr msg) {
        this->odometryCallback(msg, obs_id);
      };
      
      obstacle_subs_[obs_id] = this->create_subscription<nav_msgs::msg::Odometry>(
        topic,
        rclcpp::SensorDataQoS(),
        callback
      );
    }
  }

  RCLCPP_INFO(this->get_logger(), "Obstacle Tracker ready");
}

void ObstacleTrackerNode::odometryCallback(
  nav_msgs::msg::Odometry::SharedPtr msg,
  const std::string & obstacle_id)
{
  std::lock_guard<std::mutex> lock(mutex_);

  TrackedObstacle obstacle;
  obstacle.id = obstacle_id;
  obstacle.pose = msg->pose.pose;
  obstacle.velocity = msg->twist.twist;
  obstacle.radius = 0.5f;
  obstacle.classification = 2;
  obstacle.last_update = this->now();

  obstacles_[obstacle_id] = obstacle;
  
  RCLCPP_DEBUG(
    this->get_logger(),
    "Updated obstacle '%s': position [%.2f, %.2f]",
    obstacle_id.c_str(),
    obstacle.pose.position.x,
    obstacle.pose.position.y);
}

void ObstacleTrackerNode::publishObstacles()
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto now = this->now();
  for (auto it = obstacles_.begin(); it != obstacles_.end(); ) {
    if ((now - it->second.last_update).seconds() > obstacle_timeout_) {
      RCLCPP_DEBUG(this->get_logger(), "Removing stale obstacle: %s", it->first.c_str());
      it = obstacles_.erase(it);
    } else {
      ++it;
    }
  }

  swarm_nav_msgs::msg::ObstacleArray obstacle_array;
  obstacle_array.header.stamp = now;
  obstacle_array.header.frame_id = "map";

  for (const auto & [id, obstacle] : obstacles_) {
    swarm_nav_msgs::msg::Obstacle obs_msg;
    obs_msg.id = obstacle.id;
    obs_msg.pose = obstacle.pose;
    obs_msg.velocity = obstacle.velocity;
    obs_msg.radius = obstacle.radius;
    obs_msg.classification = obstacle.classification;

    obstacle_array.obstacles.push_back(obs_msg);
  }

  obstacle_pub_->publish(obstacle_array);
  RCLCPP_DEBUG(this->get_logger(), "Published %zu obstacles", obstacles_.size());
}

} // namespace swarm_nav_navigation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::ObstacleTrackerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

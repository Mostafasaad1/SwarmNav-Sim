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

#include "swarm_nav_msgs/msg/obstacle_array.hpp"
#include "swarm_nav_msgs/msg/obstacle.hpp"

namespace swarm_nav_navigation
{

struct TrackedObstacle
{
  std::string id;
  geometry_msgs::msg::Pose pose;
  geometry_msgs::msg::Twist velocity;
  float radius;
  uint8_t classification;
  rclcpp::Time last_update;
};

class ObstacleTrackerNode : public rclcpp::Node
{
public:
  ObstacleTrackerNode()
  : Node("obstacle_tracker_node")
  {
    // Declare parameters
    this->declare_parameter("use_sim_time", true);
    this->declare_parameter("publish_rate", 10.0);
    this->declare_parameter("obstacle_timeout", 2.0);

    // Get parameters
    double publish_rate = this->get_parameter("publish_rate").as_double();
    obstacle_timeout_ = this->get_parameter("obstacle_timeout").as_double();

    RCLCPP_INFO(this->get_logger(), "Obstacle Tracker initialized");
    RCLCPP_INFO(this->get_logger(), "Publish rate: %.1f Hz", publish_rate);
    RCLCPP_INFO(this->get_logger(), "Obstacle timeout: %.1f seconds", obstacle_timeout_);

    // TODO: Subscribe to ground truth obstacle poses from simulator
    // For Isaac Sim or Gazebo, this would be model states or TF frames

    // Publisher for tracked obstacles
    obstacle_pub_ = this->create_publisher<swarm_nav_msgs::msg::ObstacleArray>(
      "/swarm/tracked_obstacles", rclcpp::SensorDataQoS()
    );

    // Timer for periodic publishing
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(1000.0 / publish_rate)),
      std::bind(&ObstacleTrackerNode::publishObstacles, this)
    );

    // Initialize some test obstacles (placeholder)
    initializeTestObstacles();

    RCLCPP_INFO(this->get_logger(), "Obstacle Tracker ready");
  }

private:
  void initializeTestObstacles()
  {
    // Placeholder: Create some test obstacles
    // In real implementation, these would come from simulator

    for (int i = 0; i < 3; ++i) {
      TrackedObstacle forklift;
      forklift.id = "forklift_" + std::to_string(i);
      forklift.pose.position.x = -10.0 + i * 10.0;
      forklift.pose.position.y = 0.0;
      forklift.pose.position.z = 0.0;
      forklift.pose.orientation.w = 1.0;
      forklift.velocity.linear.x = 0.5;
      forklift.velocity.linear.y = 0.0;
      forklift.radius = 1.0;
      forklift.classification = 1; // SEMI_DYNAMIC
      forklift.last_update = this->now();

      obstacles_[forklift.id] = forklift;
    }

    for (int i = 0; i < 5; ++i) {
      TrackedObstacle human;
      human.id = "human_" + std::to_string(i);
      human.pose.position.x = -15.0 + i * 7.0;
      human.pose.position.y = 10.0;
      human.pose.position.z = 0.0;
      human.pose.orientation.w = 1.0;
      human.velocity.linear.x = 0.3;
      human.velocity.linear.y = 0.2;
      human.radius = 0.3;
      human.classification = 2; // DYNAMIC
      human.last_update = this->now();

      obstacles_[human.id] = human;
    }

    RCLCPP_INFO(this->get_logger(), "Initialized %zu test obstacles", obstacles_.size());
  }

  void publishObstacles()
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove stale obstacles
    auto now = this->now();
    for (auto it = obstacles_.begin(); it != obstacles_.end(); ) {
      if ((now - it->second.last_update).seconds() > obstacle_timeout_) {
        RCLCPP_DEBUG(this->get_logger(), "Removing stale obstacle: %s", it->first.c_str());
        it = obstacles_.erase(it);
      } else {
        ++it;
      }
    }

    // Create and publish ObstacleArray message
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

    // Update test obstacle positions (simple simulation)
    updateTestObstacles();
  }

  void updateTestObstacles()
  {
    // Simple motion update for test obstacles
    double dt = 0.1; // Assume 10 Hz

    for (auto & [id, obstacle] : obstacles_) {
      obstacle.pose.position.x += obstacle.velocity.linear.x * dt;
      obstacle.pose.position.y += obstacle.velocity.linear.y * dt;

      // Simple boundary bounce
      if (obstacle.pose.position.x > 20.0 || obstacle.pose.position.x < -20.0) {
        obstacle.velocity.linear.x *= -1.0;
      }
      if (obstacle.pose.position.y > 30.0 || obstacle.pose.position.y < -30.0) {
        obstacle.velocity.linear.y *= -1.0;
      }

      obstacle.last_update = this->now();
    }
  }

  // Member variables
  double obstacle_timeout_;

  std::map<std::string, TrackedObstacle> obstacles_;
  std::mutex mutex_;

  rclcpp::Publisher<swarm_nav_msgs::msg::ObstacleArray>::SharedPtr obstacle_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace swarm_nav_navigation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::ObstacleTrackerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

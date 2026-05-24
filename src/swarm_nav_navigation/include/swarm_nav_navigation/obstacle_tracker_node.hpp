#ifndef SWARM_NAV_NAVIGATION__OBSTACLE_TRACKER_NODE_HPP_
#define SWARM_NAV_NAVIGATION__OBSTACLE_TRACKER_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>
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
  ObstacleTrackerNode();

private:
  void odometryCallback(nav_msgs::msg::Odometry::SharedPtr msg, const std::string & obstacle_id);
  void publishObstacles();
  void updateTestObstacles();

  double obstacle_timeout_;

  std::map<std::string, TrackedObstacle> obstacles_;
  std::map<std::string, rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr> obstacle_subs_;
  std::mutex mutex_;

  rclcpp::Publisher<swarm_nav_msgs::msg::ObstacleArray>::SharedPtr obstacle_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace swarm_nav_navigation

#endif // SWARM_NAV_NAVIGATION__OBSTACLE_TRACKER_NODE_HPP_

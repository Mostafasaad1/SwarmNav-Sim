#ifndef SWARM_NAV_NAVIGATION__ORCA_VELOCITY_FILTER_NODE_HPP_
#define SWARM_NAV_NAVIGATION__ORCA_VELOCITY_FILTER_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

#include <string>
#include <vector>
#include <mutex>

#include "swarm_nav_msgs/msg/neighbor_state_array.hpp"
#include "swarm_nav_msgs/msg/neighbor_state.hpp"

namespace swarm_nav_navigation
{

struct NeighborData
{
  std::string robot_id;
  geometry_msgs::msg::Pose pose;
  geometry_msgs::msg::Twist velocity;
  float radius;
};

class OrcaVelocityFilterNode : public rclcpp::Node
{
public:
  OrcaVelocityFilterNode();

  void cmdVelNav2Callback(geometry_msgs::msg::Twist::SharedPtr msg);
  void odomCallback(nav_msgs::msg::Odometry::SharedPtr msg);
  void neighborCallback(swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg);

  double getYaw(const geometry_msgs::msg::Quaternion & q);
  geometry_msgs::msg::Twist computeOrcaVelocity(const geometry_msgs::msg::Twist & desired_vel);
  void publishOwnState();

  double getMaxLinearVel() const { return max_linear_vel_; }
  double getMaxAngularVel() const { return max_angular_vel_; }
  double getTimeHorizon() const { return time_horizon_; }
  double getRobotRadius() const { return robot_radius_; }
  const std::string & getRobotId() const { return robot_id_; }
  const std::vector<NeighborData> & getNeighbors() const { return neighbors_; }

private:
  std::string robot_id_;
  double robot_radius_;
  int max_neighbors_;
  double time_horizon_;
  double max_linear_vel_;
  double max_angular_vel_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  geometry_msgs::msg::Twist desired_velocity_;
  nav_msgs::msg::Odometry current_odom_;
  std::vector<NeighborData> neighbors_;

  std::mutex mutex_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_nav2_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::NeighborStateArray>::SharedPtr neighbor_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::NeighborState>::SharedPtr state_pub_;
  rclcpp::TimerBase::SharedPtr state_timer_;
};

} // namespace swarm_nav_navigation

#endif // SWARM_NAV_NAVIGATION__ORCA_VELOCITY_FILTER_NODE_HPP_

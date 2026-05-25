// orca_velocity_filter_node.cpp
// ORCA (RVO2) velocity filter for collision-free multi-robot navigation

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

#include <string>
#include <vector>
#include <mutex>

#include "swarm_nav_navigation/orca_velocity_filter_node.hpp"

namespace swarm_nav_navigation
{

OrcaVelocityFilterNode::OrcaVelocityFilterNode()
: Node("orca_velocity_filter_node")
{
  this->declare_parameter("robot_id", "robot_0");
  this->declare_parameter("robot_radius", 0.25);
  this->declare_parameter("max_neighbors", 10);
  this->declare_parameter("time_horizon", 2.0);
  this->declare_parameter("max_linear_velocity", 0.5);
  this->declare_parameter("max_angular_velocity", 1.0);
  if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

  robot_id_ = this->get_parameter("robot_id").as_string();
  robot_radius_ = this->get_parameter("robot_radius").as_double();
  max_neighbors_ = this->get_parameter("max_neighbors").as_int();
  time_horizon_ = this->get_parameter("time_horizon").as_double();
  max_linear_vel_ = this->get_parameter("max_linear_velocity").as_double();
  max_angular_vel_ = this->get_parameter("max_angular_velocity").as_double();

  RCLCPP_INFO(this->get_logger(), "ORCA Velocity Filter initialized for %s", robot_id_.c_str());
  RCLCPP_INFO(
    this->get_logger(), "Robot radius: %.2f m, Time horizon: %.2f s",
    robot_radius_, time_horizon_);

  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  cmd_vel_nav2_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel_nav2",
    10,
    [this](geometry_msgs::msg::Twist::SharedPtr msg) {
      this->cmdVelNav2Callback(msg);
    });

  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    "odom",
    10,
    [this](nav_msgs::msg::Odometry::SharedPtr msg) {
      this->odomCallback(msg);
    });

  neighbor_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborStateArray>(
    "/swarm/neighbor_states",
    rclcpp::SensorDataQoS(),
    [this](swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg) {
      this->neighborCallback(msg);
    });

  cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

  state_pub_ = this->create_publisher<swarm_nav_msgs::msg::NeighborState>(
    "/swarm/robot_state", rclcpp::SensorDataQoS()
  );

  state_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(100),
    [this]() { this->publishOwnState(); });

  RCLCPP_INFO(this->get_logger(), "ORCA Velocity Filter ready");
}

void OrcaVelocityFilterNode::cmdVelNav2Callback(geometry_msgs::msg::Twist::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);

  desired_velocity_ = *msg;

  geometry_msgs::msg::Twist safe_velocity = computeOrcaVelocity(desired_velocity_);

  cmd_vel_pub_->publish(safe_velocity);

  RCLCPP_DEBUG(
    this->get_logger(),
    "Desired: [%.2f, %.2f], Safe: [%.2f, %.2f]",
    desired_velocity_.linear.x, desired_velocity_.angular.z,
    safe_velocity.linear.x, safe_velocity.angular.z);
}

void OrcaVelocityFilterNode::odomCallback(nav_msgs::msg::Odometry::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  current_odom_ = *msg;
}

void OrcaVelocityFilterNode::neighborCallback(swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);

  neighbors_.clear();
  for (const auto & neighbor_msg : msg->neighbors) {
    if (neighbor_msg.robot_id == robot_id_) {
      continue;
    }

    NeighborData neighbor;
    neighbor.robot_id = neighbor_msg.robot_id;
    neighbor.pose = neighbor_msg.pose;
    neighbor.velocity = neighbor_msg.velocity;
    neighbor.radius = neighbor_msg.radius;

    neighbors_.push_back(neighbor);
  }

  RCLCPP_DEBUG(this->get_logger(), "Updated %zu neighbor states", neighbors_.size());
}

double OrcaVelocityFilterNode::getYaw(const geometry_msgs::msg::Quaternion & q)
{
  double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
  double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  return std::atan2(siny_cosp, cosy_cosp);
}

geometry_msgs::msg::Twist OrcaVelocityFilterNode::computeOrcaVelocity(
  const geometry_msgs::msg::Twist & desired_vel)
{
  geometry_msgs::msg::Twist safe_vel = desired_vel;

  if (neighbors_.empty()) {
    return safe_vel;
  }

  geometry_msgs::msg::Pose current_pose;
  try {
    auto transform = tf_buffer_->lookupTransform(
      "map",
      robot_id_ + "/base_footprint",
      tf2::TimePointZero
    );
    current_pose.position.x = transform.transform.translation.x;
    current_pose.position.y = transform.transform.translation.y;
    current_pose.position.z = transform.transform.translation.z;
    current_pose.orientation = transform.transform.rotation;
  } catch (const tf2::TransformException & ex) {
    RCLCPP_DEBUG(this->get_logger(), "Could not transform %s to map: %s", robot_id_.c_str(), ex.what());
    return desired_vel;
  }

  double robot_yaw = getYaw(current_pose.orientation);

  double desired_vx_world = desired_vel.linear.x * std::cos(robot_yaw) - desired_vel.linear.y * std::sin(robot_yaw);
  double desired_vy_world = desired_vel.linear.x * std::sin(robot_yaw) + desired_vel.linear.y * std::cos(robot_yaw);

  double safe_vx_world = desired_vx_world;
  double safe_vy_world = desired_vy_world;

  bool avoided_any = false;

  for (const auto & neighbor : neighbors_) {
    double dx = neighbor.pose.position.x - current_pose.position.x;
    double dy = neighbor.pose.position.y - current_pose.position.y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 10.0) {continue;}

    double combined_radius = robot_radius_ + neighbor.radius;

    double neighbor_yaw = getYaw(neighbor.pose.orientation);
    double neighbor_vx_world = neighbor.velocity.linear.x * std::cos(neighbor_yaw) - neighbor.velocity.linear.y * std::sin(neighbor_yaw);
    double neighbor_vy_world = neighbor.velocity.linear.x * std::sin(neighbor_yaw) + neighbor.velocity.linear.y * std::cos(neighbor_yaw);

    bool will_collide = false;
    if (dist < combined_radius) {
      will_collide = true;
    } else {
      double rel_vx = safe_vx_world - neighbor_vx_world;
      double rel_vy = safe_vy_world - neighbor_vy_world;
      double rel_vel_sq = rel_vx * rel_vx + rel_vy * rel_vy;

      if (rel_vel_sq > 1e-6) {
        double t = (dx * rel_vx + dy * rel_vy) / rel_vel_sq;
        if (t > 0.0 && t < time_horizon_) {
          double cx = dx - rel_vx * t;
          double cy = dy - rel_vy * t;
          double dist_sq_closest = cx * cx + cy * cy;
          if (dist_sq_closest < combined_radius * combined_radius) {
            will_collide = true;
          }
        }
      }
    }

    if (will_collide) {
      avoided_any = true;

      if (dist < combined_radius) {
        double dir_x = dx / (dist > 1e-3 ? dist : 1.0);
        double dir_y = dy / (dist > 1e-3 ? dist : 1.0);
        double rel_vx_safe = -dir_x * max_linear_vel_;
        double rel_vy_safe = -dir_y * max_linear_vel_;
        safe_vx_world = neighbor_vx_world + rel_vx_safe;
        safe_vy_world = neighbor_vy_world + rel_vy_safe;
      } else {
        double rel_vx = safe_vx_world - neighbor_vx_world;
        double rel_vy = safe_vy_world - neighbor_vy_world;

        double sin_phi = combined_radius / dist;
        double phi = std::asin(std::min(1.0, sin_phi));

        double theta = std::atan2(dy, dx);

        double alpha1 = theta - phi;
        double alpha2 = theta + phi;
        double t1_x = std::cos(alpha1);
        double t1_y = std::sin(alpha1);
        double t2_x = std::cos(alpha2);
        double t2_y = std::sin(alpha2);

        double proj1 = rel_vx * t1_x + rel_vy * t1_y;
        double proj2 = rel_vx * t2_x + rel_vy * t2_y;
        proj1 = std::max(0.0, proj1);
        proj2 = std::max(0.0, proj2);

        double cand1_x = proj1 * t1_x;
        double cand1_y = proj1 * t1_y;
        double cand2_x = proj2 * t2_x;
        double cand2_y = proj2 * t2_y;

        double d1_sq = (rel_vx - cand1_x) * (rel_vx - cand1_x) + (rel_vy - cand1_y) * (rel_vy - cand1_y);
        double d2_sq = (rel_vx - cand2_x) * (rel_vx - cand2_x) + (rel_vy - cand2_y) * (rel_vy - cand2_y);

        double safe_rel_vx, safe_rel_vy;
        if (d1_sq < d2_sq) {
          safe_rel_vx = cand1_x;
          safe_rel_vy = cand1_y;
        } else {
          safe_rel_vx = cand2_x;
          safe_rel_vy = cand2_y;
        }

        safe_vx_world = neighbor_vx_world + safe_rel_vx;
        safe_vy_world = neighbor_vy_world + safe_rel_vy;
      }

      RCLCPP_WARN(
        this->get_logger(),
        "Avoiding collision with %s (dist: %.2fm)",
        neighbor.robot_id.c_str(), dist);
    }
  }

  if (avoided_any) {
    safe_vel.linear.x = safe_vx_world * std::cos(robot_yaw) + safe_vy_world * std::sin(robot_yaw);
    safe_vel.linear.y = 0.0;

    double safe_speed = std::sqrt(safe_vx_world * safe_vx_world + safe_vy_world * safe_vy_world);
    if (safe_speed > 0.05) {
      double desired_heading = std::atan2(safe_vy_world, safe_vx_world);
      double heading_error = desired_heading - robot_yaw;
      while (heading_error > M_PI) heading_error -= 2.0 * M_PI;
      while (heading_error < -M_PI) heading_error += 2.0 * M_PI;

      safe_vel.angular.z = 2.5 * heading_error;
    }
  } else {
    safe_vel = desired_vel;
  }

  double linear_mag = std::abs(safe_vel.linear.x);
  if (linear_mag > max_linear_vel_) {
    safe_vel.linear.x = std::copysign(max_linear_vel_, safe_vel.linear.x);
  }

  if (std::abs(safe_vel.angular.z) > max_angular_vel_) {
    safe_vel.angular.z = std::copysign(max_angular_vel_, safe_vel.angular.z);
  }

  return safe_vel;
}

void OrcaVelocityFilterNode::publishOwnState()
{
  std::lock_guard<std::mutex> lock(mutex_);

  geometry_msgs::msg::TransformStamped transform;
  try {
    transform = tf_buffer_->lookupTransform(
      "map",
      robot_id_ + "/base_footprint",
      tf2::TimePointZero
    );
  } catch (const tf2::TransformException & ex) {
    RCLCPP_DEBUG(this->get_logger(), "Could not transform %s to map: %s", robot_id_.c_str(), ex.what());
    return;
  }

  swarm_nav_msgs::msg::NeighborState state_msg;
  state_msg.robot_id = robot_id_;
  state_msg.pose.position.x = transform.transform.translation.x;
  state_msg.pose.position.y = transform.transform.translation.y;
  state_msg.pose.position.z = transform.transform.translation.z;
  state_msg.pose.orientation = transform.transform.rotation;
  
  state_msg.velocity = current_odom_.twist.twist;
  state_msg.radius = static_cast<float>(robot_radius_);

  state_pub_->publish(state_msg);
  RCLCPP_DEBUG(this->get_logger(), "Published own state");
}

} // namespace swarm_nav_navigation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::OrcaVelocityFilterNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

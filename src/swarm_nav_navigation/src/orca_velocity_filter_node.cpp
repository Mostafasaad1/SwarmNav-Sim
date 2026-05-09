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
  OrcaVelocityFilterNode()
  : Node("orca_velocity_filter_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("robot_radius", 0.25);
    this->declare_parameter("max_neighbors", 10);
    this->declare_parameter("time_horizon", 2.0);
    this->declare_parameter("max_linear_velocity", 0.5);
    this->declare_parameter("max_angular_velocity", 1.0);
    this->declare_parameter("use_sim_time", true);

    // Get parameters
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

    // Initialize TF buffer and listener
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Subscribe to desired velocity from Nav2
    cmd_vel_nav2_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel_nav2",
      10,
      std::bind(&OrcaVelocityFilterNode::cmdVelNav2Callback, this, std::placeholders::_1)
    );

    // Subscribe to own odometry
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "odom",
      10,
      std::bind(&OrcaVelocityFilterNode::odomCallback, this, std::placeholders::_1)
    );

    // Subscribe to neighbor states
    neighbor_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborStateArray>(
      "/swarm/neighbor_states",
      rclcpp::SensorDataQoS(),
      std::bind(&OrcaVelocityFilterNode::neighborCallback, this, std::placeholders::_1)
    );

    // Publish filtered safe velocity
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);

    // Publish own state to swarm (aggregator will collect these)
    state_pub_ = this->create_publisher<swarm_nav_msgs::msg::NeighborState>(
      "/swarm/robot_state", rclcpp::SensorDataQoS()
    );

    // Timer to publish own state
    state_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100),
      std::bind(&OrcaVelocityFilterNode::publishOwnState, this)
    );

    RCLCPP_INFO(this->get_logger(), "ORCA Velocity Filter ready");
  }

private:
  void cmdVelNav2Callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Store desired velocity from Nav2
    desired_velocity_ = *msg;

    // Apply ORCA algorithm to compute safe velocity
    geometry_msgs::msg::Twist safe_velocity = computeOrcaVelocity(desired_velocity_);

    // Publish safe velocity
    cmd_vel_pub_->publish(safe_velocity);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Desired: [%.2f, %.2f], Safe: [%.2f, %.2f]",
      desired_velocity_.linear.x, desired_velocity_.angular.z,
      safe_velocity.linear.x, safe_velocity.angular.z);
  }

  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_odom_ = *msg;
  }

  void neighborCallback(const swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Update neighbor states
    neighbors_.clear();
    for (const auto & neighbor_msg : msg->neighbors) {
      // Skip own state
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

  geometry_msgs::msg::Twist computeOrcaVelocity(const geometry_msgs::msg::Twist & desired_vel)
  {
    // Implement velocity obstacle (VO) algorithm
    // NOTE: Caller already holds mutex_, do not lock again

    geometry_msgs::msg::Twist safe_vel = desired_vel;

    // If no neighbors, return desired velocity
    if (neighbors_.empty()) {
      return safe_vel;
    }

    // Check if desired velocity is in any velocity obstacle cone
    bool in_collision = false;

    for (const auto & neighbor : neighbors_) {
      // Calculate relative position
      double dx = neighbor.pose.position.x - current_odom_.pose.pose.position.x;
      double dy = neighbor.pose.position.y - current_odom_.pose.pose.position.y;
      double dist = std::sqrt(dx * dx + dy * dy);

      // Skip if too far
      if (dist > 10.0) {continue;}

      // Combined radius for collision
      double combined_radius = robot_radius_ + neighbor.radius;

      // Check if we're in collision course
      if (dist < combined_radius * 1.5) {
        in_collision = true;

        // Calculate velocity obstacle cone
        // Cone apex is at neighbor's velocity
        double neighbor_vx = neighbor.velocity.linear.x;
        double neighbor_vy = neighbor.velocity.linear.y;

        // Calculate relative velocity
        double rel_vx = safe_vel.linear.x - neighbor_vx;
        double rel_vy = safe_vel.linear.y - neighbor_vy;

        // Calculate angle to neighbor
        double angle_to_neighbor = std::atan2(dy, dx);

        // Calculate half-angle of VO cone
        double half_angle = std::asin(std::min(1.0, combined_radius / dist));

        // Calculate relative velocity angle
        double rel_vel_angle = std::atan2(rel_vy, rel_vx);

        // Check if relative velocity is inside VO cone
        double angle_diff = std::abs(rel_vel_angle - angle_to_neighbor);
        if (angle_diff > M_PI) {angle_diff = 2 * M_PI - angle_diff;}

        if (angle_diff < half_angle) {
          // Inside VO cone - project to nearest boundary
          // Simple approach: reduce velocity magnitude
          double scale = std::max(0.0, (dist - combined_radius) / (combined_radius * 0.5));
          safe_vel.linear.x *= scale;
          safe_vel.linear.y *= scale;

          RCLCPP_WARN(
            this->get_logger(),
            "Avoiding collision with %s (%.2fm), scaling velocity to %.2f",
            neighbor.robot_id.c_str(), dist, scale);
        }
      }
    }

    // Clamp to velocity limits (from parameters)
    double linear_mag = std::sqrt(
      safe_vel.linear.x * safe_vel.linear.x +
      safe_vel.linear.y * safe_vel.linear.y);
    if (linear_mag > max_linear_vel_) {
      double scale = max_linear_vel_ / linear_mag;
      safe_vel.linear.x *= scale;
      safe_vel.linear.y *= scale;
    }

    if (std::abs(safe_vel.angular.z) > max_angular_vel_) {
      safe_vel.angular.z = std::copysign(max_angular_vel_, safe_vel.angular.z);
    }

    return safe_vel;
  }

  void publishOwnState()
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Publish NeighborState message with current pose and velocity
    swarm_nav_msgs::msg::NeighborState state_msg;
    state_msg.robot_id = robot_id_;
    state_msg.pose = current_odom_.pose.pose;
    state_msg.velocity = current_odom_.twist.twist;
    state_msg.radius = static_cast<float>(robot_radius_);

    state_pub_->publish(state_msg);
    RCLCPP_DEBUG(this->get_logger(), "Published own state");
  }

  // Member variables
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

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::OrcaVelocityFilterNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

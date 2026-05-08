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

// Forward declare custom message
// #include "swarm_nav_msgs/msg/neighbor_state_array.hpp"

namespace swarm_nav_navigation
{

struct NeighborState
{
  std::string robot_id;
  geometry_msgs::msg::Pose pose;
  geometry_msgs::msg::Twist velocity;
  float radius;
};

class OrcaVelocityFilterNode : public rclcpp::Node
{
public:
  OrcaVelocityFilterNode() : Node("orca_velocity_filter_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("robot_radius", 0.25);
    this->declare_parameter("max_neighbors", 10);
    this->declare_parameter("time_horizon", 2.0);
    this->declare_parameter("use_sim_time", true);
    
    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    robot_radius_ = this->get_parameter("robot_radius").as_double();
    max_neighbors_ = this->get_parameter("max_neighbors").as_int();
    time_horizon_ = this->get_parameter("time_horizon").as_double();
    
    RCLCPP_INFO(this->get_logger(), "ORCA Velocity Filter initialized for %s", robot_id_.c_str());
    RCLCPP_INFO(this->get_logger(), "Robot radius: %.2f m, Time horizon: %.2f s", 
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
    // neighbor_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborStateArray>(
    //   "/swarm/neighbor_states",
    //   10,
    //   std::bind(&OrcaVelocityFilterNode::neighborCallback, this, std::placeholders::_1)
    // );
    
    // Publish filtered safe velocity
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    
    // Publish own state to swarm
    // state_pub_ = this->create_publisher<swarm_nav_msgs::msg::NeighborState>(
    //   "/swarm/neighbor_states", 10
    // );
    
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
    
    RCLCPP_DEBUG(this->get_logger(), 
                 "Desired: [%.2f, %.2f], Safe: [%.2f, %.2f]",
                 desired_velocity_.linear.x, desired_velocity_.angular.z,
                 safe_velocity.linear.x, safe_velocity.angular.z);
  }
  
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_odom_ = *msg;
  }
  
  geometry_msgs::msg::Twist computeOrcaVelocity(const geometry_msgs::msg::Twist& desired_vel)
  {
    // TODO: Implement full ORCA algorithm using RVO2 library
    // For now, return a simplified version
    
    geometry_msgs::msg::Twist safe_vel = desired_vel;
    
    // Check for nearby neighbors
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& neighbor : neighbors_) {
      // Calculate relative position and velocity
      double dx = neighbor.pose.position.x - current_odom_.pose.pose.position.x;
      double dy = neighbor.pose.position.y - current_odom_.pose.pose.position.y;
      double dist = std::sqrt(dx * dx + dy * dy);
      
      // If too close, reduce velocity
      double min_dist = robot_radius_ + neighbor.radius + 0.5; // 0.5m safety margin
      if (dist < min_dist) {
        double scale = std::max(0.0, (dist - (robot_radius_ + neighbor.radius)) / 0.5);
        safe_vel.linear.x *= scale;
        safe_vel.angular.z *= scale;
        
        RCLCPP_WARN(this->get_logger(), 
                    "Close to %s (%.2fm), scaling velocity to %.2f",
                    neighbor.robot_id.c_str(), dist, scale);
      }
    }
    
    return safe_vel;
  }
  
  void publishOwnState()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // TODO: Publish NeighborState message with current pose and velocity
    RCLCPP_DEBUG(this->get_logger(), "Publishing own state");
  }

  // Member variables
  std::string robot_id_;
  double robot_radius_;
  int max_neighbors_;
  double time_horizon_;
  
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  
  geometry_msgs::msg::Twist desired_velocity_;
  nav_msgs::msg::Odometry current_odom_;
  std::vector<NeighborState> neighbors_;
  
  std::mutex mutex_;
  
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_nav2_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::TimerBase::SharedPtr state_timer_;
};

} // namespace swarm_nav_navigation

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_navigation::OrcaVelocityFilterNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

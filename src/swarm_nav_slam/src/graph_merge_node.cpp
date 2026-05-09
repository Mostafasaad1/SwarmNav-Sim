// graph_merge_node.cpp
// Node for handling pose graph exchange and optimization between robots

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include "swarm_nav_msgs/msg/neighbor_state_array.hpp"

#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace swarm_nav_slam
{

class GraphMergeNode : public rclcpp::Node
{
public:
  GraphMergeNode() : Node("graph_merge_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("rendezvous_distance", 3.0);
    this->declare_parameter("shared_graph_topic", "/mrg_slam/shared_graph");
    this->declare_parameter("use_sim_time", true);
    
    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    rendezvous_distance_ = this->get_parameter("rendezvous_distance").as_double();
    shared_graph_topic_ = this->get_parameter("shared_graph_topic").as_string();
    
    RCLCPP_INFO(this->get_logger(), "Graph Merge Node initialized for %s", robot_id_.c_str());
    RCLCPP_INFO(this->get_logger(), "Rendezvous distance: %.2f meters", rendezvous_distance_);
    
    // Initialize TF buffer and listener
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
    
    // Subscribe to neighbor states for rendezvous detection
    neighbor_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborStateArray>(
      "/swarm/neighbor_states",
      10,
      std::bind(&GraphMergeNode::neighborCallback, this, std::placeholders::_1)
    );
    
    // Publisher for merged global map
    global_map_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
      "/swarm/global_map",
      rclcpp::QoS(10).transient_local()
    );
    
    // Create timer for periodic graph exchange checks
    timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&GraphMergeNode::timerCallback, this)
    );
  }

private:
  void neighborCallback(const swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Update neighbor poses
    neighbor_poses_.clear();
    for (const auto& neighbor : msg->neighbors) {
      if (neighbor.robot_id != robot_id_) {
        neighbor_poses_[neighbor.robot_id] = neighbor.pose;
      }
    }
  }
  
  void timerCallback()
  {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Check for nearby robots
    for (const auto& [neighbor_id, neighbor_pose] : neighbor_poses_) {
      if (isRobotNearby(neighbor_id, neighbor_pose)) {
        RCLCPP_INFO(this->get_logger(), "Robot %s is nearby (< %.2fm), triggering graph exchange",
                    neighbor_id.c_str(), rendezvous_distance_);
        mergeGraphs();
        break;
      }
    }
  }
  
  bool isRobotNearby(const std::string& neighbor_id, const geometry_msgs::msg::Pose& neighbor_pose)
  {
    // Get own pose from TF
    try {
      auto transform = tf_buffer_->lookupTransform(
        "map",
        robot_id_ + "/base_footprint",
        tf2::TimePointZero
      );
      
      // Calculate Euclidean distance
      double dx = neighbor_pose.position.x - transform.transform.translation.x;
      double dy = neighbor_pose.position.y - transform.transform.translation.y;
      double distance = std::sqrt(dx * dx + dy * dy);
      
      return distance < rendezvous_distance_;
      
    } catch (const tf2::TransformException& ex) {
      RCLCPP_DEBUG(this->get_logger(), "Could not get transform: %s", ex.what());
      return false;
    }
  }
  
  void mergeGraphs()
  {
    // Placeholder implementation for pose graph merging
    // In production, this would:
    // 1. Align coordinate frames using ICP or feature matching
    // 2. Merge keyframes and edges from neighbor graphs
    // 3. Run g2o optimization on merged graph
    // 4. Update local map with optimized poses
    
    RCLCPP_INFO(this->get_logger(), "Merging pose graphs (placeholder implementation)");
    
    // For now, just log that merge would happen
    // Actual mrg_slam integration would handle the real graph merging
  }

  // Member variables
  std::string robot_id_;
  double rendezvous_distance_;
  std::string shared_graph_topic_;
  
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  
  rclcpp::TimerBase::SharedPtr timer_;
  
  std::mutex graph_mutex_;
  std::map<std::string, geometry_msgs::msg::Pose> neighbor_poses_;
  std::map<std::string, std::vector<geometry_msgs::msg::PoseStamped>> neighbor_graphs_;
  
  rclcpp::Subscription<swarm_nav_msgs::msg::NeighborStateArray>::SharedPtr neighbor_sub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr global_map_pub_;
};

} // namespace swarm_nav_slam

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_slam::GraphMergeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

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
  GraphMergeNode()
  : Node("graph_merge_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("rendezvous_distance", 3.0);
    this->declare_parameter("shared_graph_topic", "/mrg_slam/shared_graph");
    if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

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
      rclcpp::SensorDataQoS(),
      [this](swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg) {
        this->neighborCallback(msg);
      });

    // Subscribe to own map
    own_map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
      "map",
      rclcpp::QoS(10).transient_local(),
      [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        this->ownMapCallback(msg);
      });

    // Publisher for merged global map
    global_map_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
      "/swarm/global_map",
      rclcpp::QoS(10).transient_local()
    );

    // Create timer for periodic graph exchange checks
    timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      [this]() { this->timerCallback(); });
  }

private:
  void ownMapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    own_map_ = msg;
  }

  void neighborCallback(const swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(graph_mutex_);

    // Update neighbor poses
    neighbor_poses_.clear();
    for (const auto & neighbor : msg->neighbors) {
      if (neighbor.robot_id != robot_id_) {
        neighbor_poses_[neighbor.robot_id] = neighbor.pose;
      }
    }
  }

  void timerCallback()
  {
    std::lock_guard<std::mutex> lock(graph_mutex_);

    // Check for nearby robots
    for (const auto & [neighbor_id, neighbor_pose] : neighbor_poses_) {
      if (isRobotNearby(neighbor_id, neighbor_pose)) {
        RCLCPP_INFO(
          this->get_logger(), "Robot %s is nearby (< %.2fm), triggering graph merge",
          neighbor_id.c_str(), rendezvous_distance_);

        // Subscribe to neighbor's map if not already subscribed
        if (neighbor_map_subs_.find(neighbor_id) == neighbor_map_subs_.end()) {
          std::string neighbor_map_topic = neighbor_id + "/map";
          neighbor_map_subs_[neighbor_id] = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
            neighbor_map_topic,
            rclcpp::QoS(10).transient_local(),
            [this, neighbor_id](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
              std::lock_guard<std::mutex> lock(graph_mutex_);
              neighbor_maps_[neighbor_id] = msg;
              RCLCPP_INFO(this->get_logger(), "Received map from %s", neighbor_id.c_str());
            }
          );
        }

        // Attempt to merge if we have both maps
        if (own_map_ && neighbor_maps_.find(neighbor_id) != neighbor_maps_.end()) {
          mergeGraphs();
        }
        break;
      }
    }
  }

  bool isRobotNearby(
    const std::string & neighbor_id,
    const geometry_msgs::msg::Pose & neighbor_pose)
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

    } catch (const tf2::TransformException & ex) {
      RCLCPP_DEBUG(this->get_logger(), "Could not get transform: %s", ex.what());
      return false;
    }
  }

  void mergeGraphs()
  {
    // Simplified map overlay merging: cell-wise max occupancy
    if (!own_map_) {
      RCLCPP_WARN(this->get_logger(), "No own map available for merging");
      return;
    }

    // Start with own map as base
    auto merged_map = std::make_shared<nav_msgs::msg::OccupancyGrid>(*own_map_);
    merged_map->header.stamp = this->now();
    merged_map->header.frame_id = "map";

    int merge_count = 0;

    // Merge each neighbor's map
    for (const auto & [neighbor_id, neighbor_map] : neighbor_maps_) {
      if (!neighbor_map) {continue;}

      RCLCPP_INFO(this->get_logger(), "Merging map from %s", neighbor_id.c_str());

      // For each cell in neighbor's map, transform to own map frame and merge
      for (unsigned int ny = 0; ny < neighbor_map->info.height; ++ny) {
        for (unsigned int nx = 0; nx < neighbor_map->info.width; ++nx) {
          int n_idx = ny * neighbor_map->info.width + nx;
          int8_t neighbor_value = neighbor_map->data[n_idx];

          // Skip unknown cells
          if (neighbor_value == -1) {continue;}

          // Convert neighbor cell to world coordinates
          double world_x = neighbor_map->info.origin.position.x +
            (nx + 0.5) * neighbor_map->info.resolution;
          double world_y = neighbor_map->info.origin.position.y +
            (ny + 0.5) * neighbor_map->info.resolution;

          // TODO: Apply TF transform from neighbor frame to own frame
          // For now, assume same coordinate frame (simplified)

          // Convert to own map coordinates
          int mx = static_cast<int>((world_x - merged_map->info.origin.position.x) /
            merged_map->info.resolution);
          int my = static_cast<int>((world_y - merged_map->info.origin.position.y) /
            merged_map->info.resolution);

          // Check bounds
          if (mx >= 0 && mx < static_cast<int>(merged_map->info.width) &&
            my >= 0 && my < static_cast<int>(merged_map->info.height))
          {
            int m_idx = my * merged_map->info.width + mx;

            // Cell-wise max occupancy
            if (merged_map->data[m_idx] == -1) {
              merged_map->data[m_idx] = neighbor_value;
            } else {
              merged_map->data[m_idx] = std::max(merged_map->data[m_idx], neighbor_value);
            }
            merge_count++;
          }
        }
      }
    }

    // Publish merged global map
    global_map_pub_->publish(*merged_map);
    RCLCPP_INFO(
      this->get_logger(), "Published merged global map with %d cells merged",
      merge_count);
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

  nav_msgs::msg::OccupancyGrid::SharedPtr own_map_;
  std::map<std::string, nav_msgs::msg::OccupancyGrid::SharedPtr> neighbor_maps_;
  std::map<std::string,
    rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr> neighbor_map_subs_;

  rclcpp::Subscription<swarm_nav_msgs::msg::NeighborStateArray>::SharedPtr neighbor_sub_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr own_map_sub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr global_map_pub_;
};

} // namespace swarm_nav_slam

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_slam::GraphMergeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

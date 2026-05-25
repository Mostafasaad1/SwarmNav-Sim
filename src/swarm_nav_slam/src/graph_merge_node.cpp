// graph_merge_node.cpp
// Centralized Node for merging individual robot maps into a global map

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>

namespace swarm_nav_slam
{

class GraphMergeNode : public rclcpp::Node
{
public:
  GraphMergeNode()
  : Node("graph_merge_node")
  {
    // Declare parameters
    this->declare_parameter("robot_ids", std::vector<std::string>{"robot_0", "robot_1", "robot_2"});
    if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

    // Get parameters
    robot_ids_ = this->get_parameter("robot_ids").as_string_array();

    RCLCPP_INFO(this->get_logger(), "Global Graph Merge Node initialized for %zu robots", robot_ids_.size());

    // Initialize TF buffer and listener
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Subscribe to all robot maps
    for (const auto& robot_id : robot_ids_) {
      std::string topic = "/" + robot_id + "/map";
      map_subs_[robot_id] = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
        topic,
        rclcpp::QoS(10).transient_local(),
        [this, robot_id](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
          std::lock_guard<std::mutex> lock(map_mutex_);
          robot_maps_[robot_id] = msg;
          RCLCPP_INFO_ONCE(this->get_logger(), "Received first map from %s", robot_id.c_str());
        });
    }

    // Publisher for merged global map
    global_map_pub_ = this->create_publisher<nav_msgs::msg::OccupancyGrid>(
      "/swarm/global_map",
      rclcpp::QoS(10).transient_local()
    );

    // Create timer for periodic map merging
    timer_ = this->create_wall_timer(
      std::chrono::seconds(2),
      [this]() { this->mergeGraphs(); });
  }

private:
  void mergeGraphs()
  {
    std::lock_guard<std::mutex> lock(map_mutex_);

    if (robot_maps_.empty()) {
      return;
    }

    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();

    double resolution = 0.05; // Default

    // First pass: find the global bounding box
    std::map<std::string, geometry_msgs::msg::TransformStamped> transforms;
    
    for (const auto& [robot_id, map_msg] : robot_maps_) {
      if (!map_msg) continue;
      resolution = map_msg->info.resolution;

      try {
        auto transform = tf_buffer_->lookupTransform(
          "map",
          robot_id + "/map",
          tf2::TimePointZero
        );
        transforms[robot_id] = transform;

        // Origin of this map in its own frame
        double local_ox = map_msg->info.origin.position.x;
        double local_oy = map_msg->info.origin.position.y;
        double local_max_x = local_ox + map_msg->info.width * resolution;
        double local_max_y = local_oy + map_msg->info.height * resolution;

        // Transform corners to global frame
        geometry_msgs::msg::Pose p1, p2, p3, p4;
        p1.position.x = local_ox; p1.position.y = local_oy;
        p2.position.x = local_max_x; p2.position.y = local_oy;
        p3.position.x = local_ox; p3.position.y = local_max_y;
        p4.position.x = local_max_x; p4.position.y = local_max_y;

        geometry_msgs::msg::Pose p1_t, p2_t, p3_t, p4_t;
        tf2::doTransform(p1, p1_t, transform);
        tf2::doTransform(p2, p2_t, transform);
        tf2::doTransform(p3, p3_t, transform);
        tf2::doTransform(p4, p4_t, transform);

        min_x = std::min({min_x, p1_t.position.x, p2_t.position.x, p3_t.position.x, p4_t.position.x});
        min_y = std::min({min_y, p1_t.position.y, p2_t.position.y, p3_t.position.y, p4_t.position.y});
        max_x = std::max({max_x, p1_t.position.x, p2_t.position.x, p3_t.position.x, p4_t.position.x});
        max_y = std::max({max_y, p1_t.position.y, p2_t.position.y, p3_t.position.y, p4_t.position.y});

      } catch (const tf2::TransformException & ex) {
        RCLCPP_WARN(this->get_logger(), "Could not get transform for %s: %s", robot_id.c_str(), ex.what());
      }
    }

    if (transforms.empty()) {
      return; // No valid transforms yet
    }

    // Create the global map
    auto merged_map = std::make_shared<nav_msgs::msg::OccupancyGrid>();
    merged_map->header.stamp = this->now();
    merged_map->header.frame_id = "map";
    merged_map->info.resolution = resolution;
    merged_map->info.origin.position.x = min_x;
    merged_map->info.origin.position.y = min_y;
    merged_map->info.origin.orientation.w = 1.0;

    int width = std::ceil((max_x - min_x) / resolution);
    int height = std::ceil((max_y - min_y) / resolution);

    if (width <= 0 || height <= 0) return;

    merged_map->info.width = width;
    merged_map->info.height = height;
    merged_map->data.assign(width * height, -1);

    int merge_count = 0;

    // Second pass: merge cells
    for (const auto& [robot_id, map_msg] : robot_maps_) {
      if (!map_msg || transforms.find(robot_id) == transforms.end()) continue;
      
      auto& transform = transforms[robot_id];

      for (unsigned int ny = 0; ny < map_msg->info.height; ++ny) {
        for (unsigned int nx = 0; nx < map_msg->info.width; ++nx) {
          int n_idx = ny * map_msg->info.width + nx;
          int8_t neighbor_value = map_msg->data[n_idx];

          if (neighbor_value == -1) continue;

          // Local coordinate
          double local_x = map_msg->info.origin.position.x + (nx + 0.5) * resolution;
          double local_y = map_msg->info.origin.position.y + (ny + 0.5) * resolution;

          geometry_msgs::msg::Pose local_pose, global_pose;
          local_pose.position.x = local_x;
          local_pose.position.y = local_y;
          tf2::doTransform(local_pose, global_pose, transform);

          // Global map index
          int mx = static_cast<int>((global_pose.position.x - min_x) / resolution);
          int my = static_cast<int>((global_pose.position.y - min_y) / resolution);

          if (mx >= 0 && mx < width && my >= 0 && my < height) {
            int m_idx = my * width + mx;
            
            // Cell-wise max occupancy (pessimistic merging)
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
    RCLCPP_DEBUG(this->get_logger(), "Published merged global map with %d cells", merge_count);
  }

  std::vector<std::string> robot_ids_;
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  std::mutex map_mutex_;
  std::map<std::string, nav_msgs::msg::OccupancyGrid::SharedPtr> robot_maps_;
  std::map<std::string, rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr> map_subs_;
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

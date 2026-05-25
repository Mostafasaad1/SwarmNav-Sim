// Copyright 2026 SwarmNav-Sim Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// frontier_detector_node.cpp
// Detects unexplored frontiers in the occupancy grid map.
// Large frontier clusters (rings) are split into angular sectors so that
// robots get meaningful navigation targets at the map boundary.

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

#include "swarm_nav_msgs/msg/frontier.hpp"
#include "swarm_nav_msgs/msg/frontier_array.hpp"

namespace swarm_nav_coordination
{

struct FrontierData
{
  geometry_msgs::msg::Point centroid;
  float size;
  float utility;
  std::string frontier_id;
};

class FrontierDetectorNode : public rclcpp::Node
{
public:
  FrontierDetectorNode()
  : Node("frontier_detector_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("min_frontier_size", 5);
    this->declare_parameter("frontier_travel_point_distance", 2.0);  // look-ahead beyond frontier
    if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    min_frontier_size_ = this->get_parameter("min_frontier_size").as_int();
    frontier_travel_distance_ = this->get_parameter("frontier_travel_point_distance").as_double();

    // TF for robot pose lookup (used for look-ahead offset direction)
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    RCLCPP_INFO(this->get_logger(), "Frontier Detector initialized for %s", robot_id_.c_str());

    // Subscribe to local map
    map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
      "map",
      rclcpp::QoS(10).transient_local(),
      [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        this->mapCallback(msg);
      });

    // Publisher for detected frontiers
    frontier_pub_ = this->create_publisher<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers", 10
    );

    // Publisher for visualization
    marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
      "frontier_markers", 10
    );

    // Timer to continuously publish latest known frontiers
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(1000),
      [this]() {
        if (!latest_frontiers_.empty()) {
          publishFrontiers(latest_frontiers_);
          publishMarkers(latest_frontiers_);
        }
      });

    RCLCPP_INFO(this->get_logger(), "Frontier Detector ready");
  }

private:
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    RCLCPP_DEBUG(this->get_logger(), "Received map update");

    // Store latest map for utility calculation
    latest_map_ = map;

    // Get robot position in local map frame (for look-ahead direction)
    robot_x_in_map_ = 0.0;
    robot_y_in_map_ = 0.0;
    try {
      auto tf = tf_buffer_->lookupTransform(
        robot_id_ + "/map",
        robot_id_ + "/base_footprint",
        tf2::TimePointZero);
      robot_x_in_map_ = tf.transform.translation.x;
      robot_y_in_map_ = tf.transform.translation.y;
    } catch (const std::exception & e) {
      RCLCPP_DEBUG(this->get_logger(), "TF lookup failed, using (0,0): %s", e.what());
    }

    // Detect frontiers
    std::vector<FrontierData> frontiers = detectFrontiers(map);

    RCLCPP_INFO(this->get_logger(), "Detected %zu frontiers", frontiers.size());

    // Cache latest frontiers for timer
    latest_frontiers_ = frontiers;

    // Publish immediately on map update
    publishFrontiers(frontiers);
    publishMarkers(frontiers);
  }

  // ---------------------------------------------------------------
  // Core frontier detection with angular sector splitting
  // ---------------------------------------------------------------
  std::vector<FrontierData> detectFrontiers(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    std::vector<FrontierData> frontiers;

    int width  = map->info.width;
    int height = map->info.height;
    float resolution = map->info.resolution;
    auto origin = map->info.origin.position;

    // --- Step 1: Find connected frontier clusters via BFS ---
    std::vector<bool> visited(width * height, false);

    // Each raw cluster is just a list of grid cells
    std::vector<std::vector<std::pair<int, int>>> raw_clusters;

    for (int y = 1; y < height - 1; ++y) {
      for (int x = 1; x < width - 1; ++x) {
        int idx = y * width + x;
        if (visited[idx]) { continue; }
        if (!isFrontierCell(map, x, y)) { continue; }

        // BFS to collect all connected frontier cells
        std::vector<std::pair<int, int>> cluster_cells;
        std::queue<std::pair<int, int>> bfs;
        bfs.push({x, y});
        visited[idx] = true;

        while (!bfs.empty()) {
          auto [cx, cy] = bfs.front();
          bfs.pop();
          cluster_cells.push_back({cx, cy});

          for (auto [dx, dy] : std::vector<std::pair<int, int>>{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}) {
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx >= 1 && nx < width - 1 && ny >= 1 && ny < height - 1) {
              int nidx = ny * width + nx;
              if (!visited[nidx] && isFrontierCell(map, nx, ny)) {
                visited[nidx] = true;
                bfs.push({nx, ny});
              }
            }
          }
        }

        if (cluster_cells.size() >= static_cast<size_t>(min_frontier_size_)) {
          raw_clusters.push_back(std::move(cluster_cells));
        }

      }
    }

    // --- Step 2: Process clusters — split large ones by angular sector ---
    constexpr int    NUM_SECTORS     = 8;
    constexpr double SECTOR_SIZE     = 2.0 * M_PI / NUM_SECTORS;
    constexpr size_t SPLIT_THRESHOLD = 40;  // clusters bigger than this get split

    for (auto & cells : raw_clusters) {
      if (cells.size() <= SPLIT_THRESHOLD) {
        // Small / medium cluster — keep as a single frontier
        FrontierData fd = buildFrontierFromCells(map, cells);
        fd.utility    = calculateUtility(fd);
        fd.frontier_id = generateFrontierId(fd.centroid);
        frontiers.push_back(fd);
      } else {
        // Large cluster — likely a ring or long arc.  Split by angle.
        // 1. Compute geometric center of the cluster
        double cx_sum = 0.0, cy_sum = 0.0;
        for (auto [gx, gy] : cells) {
          cx_sum += origin.x + gx * resolution;
          cy_sum += origin.y + gy * resolution;
        }
        double center_x = cx_sum / cells.size();
        double center_y = cy_sum / cells.size();

        // 2. Assign each cell to an angular sector
        std::vector<std::vector<std::pair<int, int>>> sectors(NUM_SECTORS);
        for (auto [gx, gy] : cells) {
          double wx = origin.x + gx * resolution;
          double wy = origin.y + gy * resolution;
          double angle = std::atan2(wy - center_y, wx - center_x);
          if (angle < 0) { angle += 2.0 * M_PI; }
          int sector = std::min(static_cast<int>(angle / SECTOR_SIZE), NUM_SECTORS - 1);
          sectors[sector].push_back({gx, gy});
        }

        // 3. Each non-trivial sector → its own frontier
        int min_sector_size = std::max(3, min_frontier_size_ / 2);
        for (auto & sector_cells : sectors) {
          if (static_cast<int>(sector_cells.size()) >= min_sector_size) {
            FrontierData fd = buildFrontierFromCells(map, sector_cells);
            fd.utility    = calculateUtility(fd);
            fd.frontier_id = generateFrontierId(fd.centroid);
            frontiers.push_back(fd);
          }
        }
      }
    }

    return frontiers;
  }

  // ---------------------------------------------------------------
  // Build a FrontierData from a set of grid cells
  // ---------------------------------------------------------------
  FrontierData buildFrontierFromCells(
    const nav_msgs::msg::OccupancyGrid::SharedPtr map,
    const std::vector<std::pair<int, int>> & cells)
  {
    FrontierData frontier;
    float resolution = map->info.resolution;
    auto origin = map->info.origin.position;
    int width  = map->info.width;
    int height = static_cast<int>(map->info.height);

    // 1. Compute centroid in world (map-frame) coordinates
    double sum_x = 0.0, sum_y = 0.0;
    for (auto [gx, gy] : cells) {
      sum_x += origin.x + gx * resolution;
      sum_y += origin.y + gy * resolution;
    }
    double centroid_x = sum_x / cells.size();
    double centroid_y = sum_y / cells.size();

    // 2. Check if centroid lands in free space; if not fall back to closest frontier cell
    int check_gx = static_cast<int>((centroid_x - origin.x) / resolution);
    int check_gy = static_cast<int>((centroid_y - origin.y) / resolution);
    bool centroid_free = false;
    if (check_gx >= 0 && check_gx < width && check_gy >= 0 && check_gy < height) {
      centroid_free = (map->data[check_gy * width + check_gx] == 0);
    }

    if (!centroid_free) {
      // Centroid is in unknown / occupied space.
      // Fall back to the frontier cell closest to the computed centroid.
      double best_dist = 1e9;
      for (auto [gx, gy] : cells) {
        double wx = origin.x + gx * resolution;
        double wy = origin.y + gy * resolution;
        double dist = std::hypot(wx - centroid_x, wy - centroid_y);
        if (dist < best_dist) {
          best_dist  = dist;
          centroid_x = wx;
          centroid_y = wy;
        }
      }
    }

    frontier.centroid.x = centroid_x;
    frontier.centroid.y = centroid_y;
    frontier.centroid.z = 0.0;
    frontier.size = cells.size();

    // 3. Apply look-ahead: push goal beyond frontier boundary into unexplored space.
    //    Direction = centroid - robot_position; then offset by frontier_travel_distance_.
    {
      double dx = frontier.centroid.x - robot_x_in_map_;
      double dy = frontier.centroid.y - robot_y_in_map_;
      double dist = std::sqrt(dx * dx + dy * dy);
      if (dist > 0.01) {
        frontier.centroid.x += (dx / dist) * frontier_travel_distance_;
        frontier.centroid.y += (dy / dist) * frontier_travel_distance_;
      }
    }

    return frontier;
  }

  // ---------------------------------------------------------------
  // Helpers (unchanged logic)
  // ---------------------------------------------------------------
  bool isFrontierCell(const nav_msgs::msg::OccupancyGrid::SharedPtr map, int x, int y)
  {
    int width = map->info.width;
    int idx = y * width + x;

    // Cell must be free (0)
    if (map->data[idx] != 0) { return false; }

    // Check if any neighbor is unknown (-1)
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) { continue; }

        int nx = x + dx;
        int ny = y + dy;
        int nidx = ny * width + nx;

        if (map->data[nidx] == -1) {
          return true;
        }
      }
    }

    return false;
  }

  float calculateUtility(const FrontierData & frontier)
  {
    // utility = size * 0.1 + info_gain * 0.5
    // info_gain = count of unknown cells within 3 m radius of centroid

    float size_component = frontier.size * 0.1f;
    float info_gain = 0.0f;

    if (latest_map_) {
      float radius = 3.0f;
      float resolution = latest_map_->info.resolution;
      auto origin = latest_map_->info.origin.position;
      int width  = latest_map_->info.width;
      int height = latest_map_->info.height;

      int center_x = static_cast<int>((frontier.centroid.x - origin.x) / resolution);
      int center_y = static_cast<int>((frontier.centroid.y - origin.y) / resolution);

      int radius_cells = static_cast<int>(radius / resolution);
      int unknown_count = 0;

      for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
        for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
          float dist = std::sqrt(dx * dx + dy * dy) * resolution;
          if (dist > radius) { continue; }

          int gx = center_x + dx;
          int gy = center_y + dy;

          if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
            int idx = gy * width + gx;
            if (latest_map_->data[idx] == -1) {
              unknown_count++;
            }
          }
        }
      }

      info_gain = static_cast<float>(unknown_count);
    }

    return size_component + info_gain * 0.5f;
  }

  std::string generateFrontierId(const geometry_msgs::msg::Point & centroid)
  {
    // Quantize position to 1m grid for stable IDs
    int grid_x = static_cast<int>(std::round(centroid.x));
    int grid_y = static_cast<int>(std::round(centroid.y));
    return robot_id_ + "_frontier_" + std::to_string(grid_x) + "_" + std::to_string(grid_y);
  }

  void publishFrontiers(const std::vector<FrontierData> & frontiers)
  {
    swarm_nav_msgs::msg::FrontierArray frontier_array;
    frontier_array.header.stamp = this->now();
    frontier_array.header.frame_id = robot_id_ + "/map";

    for (const auto & frontier : frontiers) {
      swarm_nav_msgs::msg::Frontier frontier_msg;
      frontier_msg.centroid = frontier.centroid;
      frontier_msg.size = frontier.size;
      frontier_msg.utility = frontier.utility;
      frontier_msg.frontier_id = frontier.frontier_id;

      frontier_array.frontiers.push_back(frontier_msg);
    }

    frontier_pub_->publish(frontier_array);
    RCLCPP_DEBUG(this->get_logger(), "Published %zu frontiers", frontiers.size());
  }

  void publishMarkers(const std::vector<FrontierData> & frontiers)
  {
    visualization_msgs::msg::MarkerArray marker_array;

    visualization_msgs::msg::Marker delete_all;
    delete_all.action = visualization_msgs::msg::Marker::DELETEALL;
    marker_array.markers.push_back(delete_all);

    for (size_t i = 0; i < frontiers.size(); ++i) {
      visualization_msgs::msg::Marker marker;
      marker.header.frame_id = robot_id_ + "/map";
      marker.header.stamp = this->now();
      marker.ns = "frontiers";
      marker.id = i;
      marker.type = visualization_msgs::msg::Marker::SPHERE;
      marker.action = visualization_msgs::msg::Marker::ADD;

      marker.pose.position = frontiers[i].centroid;
      marker.pose.orientation.w = 1.0;

      marker.scale.x = 0.5;
      marker.scale.y = 0.5;
      marker.scale.z = 0.5;

      marker.color.r = 1.0;
      marker.color.g = 0.5;
      marker.color.b = 0.0;
      marker.color.a = 0.8;

      marker_array.markers.push_back(marker);
    }

    marker_pub_->publish(marker_array);
  }

  // Member variables
  std::string robot_id_;
  int min_frontier_size_;
  double frontier_travel_distance_;

  double robot_x_in_map_{0.0};
  double robot_y_in_map_{0.0};

  nav_msgs::msg::OccupancyGrid::SharedPtr latest_map_;
  std::vector<FrontierData> latest_frontiers_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace swarm_nav_coordination

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_coordination::FrontierDetectorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

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
// Detects unexplored frontiers in the occupancy grid map

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <string>
#include <vector>
#include <queue>
#include <cmath>

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
    this->declare_parameter("min_frontier_size", 10);
    this->declare_parameter("frontier_travel_point_distance", 1.0);
    if (!this->has_parameter("use_sim_time")) { this->declare_parameter("use_sim_time", true); }

    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    min_frontier_size_ = this->get_parameter("min_frontier_size").as_int();
    frontier_travel_distance_ = this->get_parameter("frontier_travel_point_distance").as_double();

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

    // Detect frontiers using wavefront algorithm
    std::vector<FrontierData> frontiers = detectFrontiers(map);

    RCLCPP_INFO(this->get_logger(), "Detected %zu frontiers", frontiers.size());

    // Cache latest frontiers for timer
    latest_frontiers_ = frontiers;

    // Publish immediately on map update
    publishFrontiers(frontiers);
    publishMarkers(frontiers);
  }

  std::vector<FrontierData> detectFrontiers(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    std::vector<FrontierData> frontiers;

    int width = map->info.width;
    int height = map->info.height;
    float resolution = map->info.resolution;

    // Create visited map
    std::vector<bool> visited(width * height, false);

    // Scan for frontier cells (free cells adjacent to unknown cells)
    for (int y = 1; y < height - 1; ++y) {
      for (int x = 1; x < width - 1; ++x) {
        int idx = y * width + x;

        // Skip if already visited
        if (visited[idx]) {continue;}

        // Check if this is a frontier cell
        if (isFrontierCell(map, x, y)) {
          // Perform BFS to find connected frontier region
          FrontierData frontier = extractFrontier(map, x, y, visited);

          // Only keep frontiers above minimum size
          if (frontier.size >= min_frontier_size_) {
            // Calculate utility (information gain estimate)
            frontier.utility = calculateUtility(frontier);

            // Generate unique ID
            frontier.frontier_id = generateFrontierId();

            frontiers.push_back(frontier);
          }
        }
      }
    }

    return frontiers;
  }

  bool isFrontierCell(const nav_msgs::msg::OccupancyGrid::SharedPtr map, int x, int y)
  {
    int width = map->info.width;
    int idx = y * width + x;

    // Cell must be free (0)
    if (map->data[idx] != 0) {return false;}

    // Check if any neighbor is unknown (-1)
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) {continue;}

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

  FrontierData extractFrontier(
    const nav_msgs::msg::OccupancyGrid::SharedPtr map,
    int start_x, int start_y,
    std::vector<bool> & visited)
  {
    FrontierData frontier;
    std::queue<std::pair<int, int>> queue;
    std::vector<std::pair<int, int>> cells;

    int width = map->info.width;
    float resolution = map->info.resolution;
    auto origin = map->info.origin.position;

    queue.push({start_x, start_y});
    visited[start_y * width + start_x] = true;

    // BFS to find all connected frontier cells
    while (!queue.empty()) {
      auto [x, y] = queue.front();
      queue.pop();

      cells.push_back({x, y});

      // Check 4-connected neighbors
      for (auto [dx, dy] : std::vector<std::pair<int, int>>{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}) {
        int nx = x + dx;
        int ny = y + dy;
        int nidx = ny * width + nx;

        if (nx >= 0 && nx < width && ny >= 0 && ny < static_cast<int>(map->info.height) &&
          !visited[nidx] && isFrontierCell(map, nx, ny))
        {
          visited[nidx] = true;
          queue.push({nx, ny});
        }
      }
    }

    // Calculate centroid
    float sum_x = 0, sum_y = 0;
    for (auto [x, y] : cells) {
      sum_x += x;
      sum_y += y;
    }

    // Calculate direction towards explored (free) space
    double dx_sum = 0.0;
    double dy_sum = 0.0;
    for (auto [x, y] : cells) {
      for (int dy_offset = -1; dy_offset <= 1; ++dy_offset) {
        for (int dx_offset = -1; dx_offset <= 1; ++dx_offset) {
          if (dx_offset == 0 && dy_offset == 0) {continue;}
          int nx = x + dx_offset;
          int ny = y + dy_offset;
          if (nx >= 0 && nx < width && ny >= 0 && ny < static_cast<int>(map->info.height)) {
            int nidx = ny * width + nx;
            if (map->data[nidx] == 0) { // Free/explored cell
              dx_sum += dx_offset;
              dy_sum += dy_offset;
            }
          }
        }
      }
    }

    double len = std::sqrt(dx_sum * dx_sum + dy_sum * dy_sum);
    double ux = 0.0;
    double uy = 0.0;
    if (len > 1e-5) {
      ux = dx_sum / len;
      uy = dy_sum / len;
    }

    frontier.centroid.x = origin.x + (sum_x / cells.size()) * resolution + ux * frontier_travel_distance_;
    frontier.centroid.y = origin.y + (sum_y / cells.size()) * resolution + uy * frontier_travel_distance_;
    frontier.centroid.z = 0.0;
    frontier.size = cells.size();

    return frontier;
  }

  float calculateUtility(const FrontierData & frontier)
  {
    // Spec formula: utility = size * 0.1 + info_gain * 0.5
    // info_gain = count of unknown cells within 3m radius of centroid

    float size_component = frontier.size * 0.1f;
    float info_gain = 0.0f;

    if (latest_map_) {
      // Calculate information gain: count unknown cells within 3m radius
      float radius = 3.0f; // meters
      float resolution = latest_map_->info.resolution;
      auto origin = latest_map_->info.origin.position;
      int width = latest_map_->info.width;
      int height = latest_map_->info.height;

      // Convert centroid to grid coordinates
      int center_x = static_cast<int>((frontier.centroid.x - origin.x) / resolution);
      int center_y = static_cast<int>((frontier.centroid.y - origin.y) / resolution);

      // Search within radius
      int radius_cells = static_cast<int>(radius / resolution);
      int unknown_count = 0;

      for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
        for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
          // Check if within circular radius
          float dist = std::sqrt(dx * dx + dy * dy) * resolution;
          if (dist > radius) {continue;}

          int x = center_x + dx;
          int y = center_y + dy;

          // Check bounds
          if (x >= 0 && x < width && y >= 0 && y < height) {
            int idx = y * width + x;
            if (latest_map_->data[idx] == -1) {
              unknown_count++;
            }
          }
        }
      }

      info_gain = static_cast<float>(unknown_count);
    }

    float utility = size_component + info_gain * 0.5f;
    return utility;
  }

  std::string generateFrontierId()
  {
    static int counter = 0;
    return robot_id_ + "_frontier_" + std::to_string(counter++);
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

  nav_msgs::msg::OccupancyGrid::SharedPtr latest_map_;
  std::vector<FrontierData> latest_frontiers_;

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

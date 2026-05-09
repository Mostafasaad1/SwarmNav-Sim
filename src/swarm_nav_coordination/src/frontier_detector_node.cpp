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

struct Frontier
{
  geometry_msgs::msg::Point centroid;
  float size;
  float utility;
  std::string frontier_id;
};

class FrontierDetectorNode : public rclcpp::Node
{
public:
  FrontierDetectorNode() : Node("frontier_detector_node")
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("min_frontier_size", 10);
    this->declare_parameter("frontier_travel_point_distance", 1.0);
    this->declare_parameter("use_sim_time", true);
    
    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    min_frontier_size_ = this->get_parameter("min_frontier_size").as_int();
    frontier_travel_distance_ = this->get_parameter("frontier_travel_point_distance").as_double();
    
    RCLCPP_INFO(this->get_logger(), "Frontier Detector initialized for %s", robot_id_.c_str());
    
    // Subscribe to local map
    map_sub_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
      "map",
      rclcpp::QoS(10).transient_local(),
      std::bind(&FrontierDetectorNode::mapCallback, this, std::placeholders::_1)
    );
    
    // Publisher for detected frontiers
    frontier_pub_ = this->create_publisher<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers", 10
    );
    
    // Publisher for visualization
    marker_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
      "frontier_markers", 10
    );
    
    RCLCPP_INFO(this->get_logger(), "Frontier Detector ready");
  }

private:
  void mapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    RCLCPP_DEBUG(this->get_logger(), "Received map update");
    
    // Detect frontiers using wavefront algorithm
    std::vector<Frontier> frontiers = detectFrontiers(map);
    
    RCLCPP_INFO(this->get_logger(), "Detected %zu frontiers", frontiers.size());
    
    // Publish frontiers
    publishFrontiers(frontiers);
    
    // Publish visualization markers
    publishMarkers(frontiers);
  }
  
  std::vector<Frontier> detectFrontiers(const nav_msgs::msg::OccupancyGrid::SharedPtr map)
  {
    std::vector<Frontier> frontiers;
    
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
        if (visited[idx]) continue;
        
        // Check if this is a frontier cell
        if (isFrontierCell(map, x, y)) {
          // Perform BFS to find connected frontier region
          Frontier frontier = extractFrontier(map, x, y, visited);
          
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
    if (map->data[idx] != 0) return false;
    
    // Check if any neighbor is unknown (-1)
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        
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
  
  Frontier extractFrontier(const nav_msgs::msg::OccupancyGrid::SharedPtr map, 
                          int start_x, int start_y, 
                          std::vector<bool>& visited)
  {
    Frontier frontier;
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
      for (auto [dx, dy] : std::vector<std::pair<int, int>>{{-1,0}, {1,0}, {0,-1}, {0,1}}) {
        int nx = x + dx;
        int ny = y + dy;
        int nidx = ny * width + nx;
        
        if (nx >= 0 && nx < width && ny >= 0 && ny < (int)map->info.height &&
            !visited[nidx] && isFrontierCell(map, nx, ny)) {
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
    
    frontier.centroid.x = origin.x + (sum_x / cells.size()) * resolution;
    frontier.centroid.y = origin.y + (sum_y / cells.size()) * resolution;
    frontier.centroid.z = 0.0;
    frontier.size = cells.size();
    
    return frontier;
  }
  
  float calculateUtility(const Frontier& frontier)
  {
    // Simple utility: proportional to frontier size
    // More sophisticated: consider distance, information gain, etc.
    return frontier.size;
  }
  
  std::string generateFrontierId()
  {
    static int counter = 0;
    return robot_id_ + "_frontier_" + std::to_string(counter++);
  }
  
  void publishFrontiers(const std::vector<Frontier>& frontiers)
  {
    swarm_nav_msgs::msg::FrontierArray frontier_array;
    frontier_array.header.stamp = this->now();
    frontier_array.header.frame_id = "map";
    
    for (const auto& frontier : frontiers) {
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
  
  void publishMarkers(const std::vector<Frontier>& frontiers)
  {
    visualization_msgs::msg::MarkerArray marker_array;
    
    for (size_t i = 0; i < frontiers.size(); ++i) {
      visualization_msgs::msg::Marker marker;
      marker.header.frame_id = "map";
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
  
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;
};

} // namespace swarm_nav_coordination

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_coordination::FrontierDetectorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

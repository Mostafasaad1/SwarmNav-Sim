// dynamic_obstacle_layer.cpp
// Implementation of custom Nav2 costmap layer for dynamic obstacle avoidance

#include "dynamic_obstacle_layer.hpp"
#include <nav2_costmap_2d/costmap_math.hpp>
#include <pluginlib/class_list_macros.hpp>
#include "swarm_nav_msgs/msg/obstacle_array.hpp"
#include <rclcpp/serialization.hpp>

PLUGINLIB_EXPORT_CLASS(swarm_nav_navigation::DynamicObstacleLayer, nav2_costmap_2d::Layer)

namespace swarm_nav_navigation
{

DynamicObstacleLayer::DynamicObstacleLayer()
: inflation_radius_(1.0),
  prediction_time_(2.0),
  enabled_(true)
{
}

DynamicObstacleLayer::~DynamicObstacleLayer()
{
}

void DynamicObstacleLayer::onInitialize()
{
  auto node = node_.lock();
  if (!node) {
    throw std::runtime_error("Unable to lock node!");
  }

  // Declare parameters
  declareParameter("enabled", rclcpp::ParameterValue(true));
  declareParameter("inflation_radius", rclcpp::ParameterValue(1.0));
  declareParameter("prediction_time", rclcpp::ParameterValue(2.0));

  // Get parameters
  node->get_parameter(name_ + "." + "enabled", enabled_);
  node->get_parameter(name_ + "." + "inflation_radius", inflation_radius_);
  node->get_parameter(name_ + "." + "prediction_time", prediction_time_);

  RCLCPP_INFO(
    node->get_logger(),
    "DynamicObstacleLayer initialized: inflation_radius=%.2f, prediction_time=%.2f",
    inflation_radius_, prediction_time_);

  // Subscribe to tracked obstacles
  obstacle_sub_ = node->create_subscription<rclcpp::SerializedMessage>(
    "/swarm/tracked_obstacles",
    rclcpp::SensorDataQoS(),
    std::bind(&DynamicObstacleLayer::obstacleCallback, this, std::placeholders::_1));

  current_ = true;
}

void DynamicObstacleLayer::obstacleCallback(
  const std::shared_ptr<rclcpp::SerializedMessage> msg)
{
  std::lock_guard<std::mutex> lock(obstacles_mutex_);
  
  // Deserialize ObstacleArray message
  rclcpp::Serialization<swarm_nav_msgs::msg::ObstacleArray> serializer;
  swarm_nav_msgs::msg::ObstacleArray obstacle_array;
  
  try {
    serializer.deserialize_message(msg.get(), &obstacle_array);
    
    // Clear old obstacles and update with new data
    obstacles_.clear();
    
    for (const auto& obs_msg : obstacle_array.obstacles) {
      DynamicObstacle obstacle;
      obstacle.id = obs_msg.id;
      obstacle.pose = obs_msg.pose;
      obstacle.velocity = obs_msg.velocity;
      obstacle.radius = obs_msg.radius;
      obstacle.classification = obs_msg.classification;
      
      obstacles_.push_back(obstacle);
    }
    
    RCLCPP_DEBUG(node_.lock()->get_logger(), 
                 "Received %zu obstacles", obstacles_.size());
  } catch (const std::exception& e) {
    RCLCPP_ERROR(node_.lock()->get_logger(), 
                 "Failed to deserialize obstacle message: %s", e.what());
  }
}

void DynamicObstacleLayer::updateBounds(
  double robot_x, double robot_y, double robot_yaw,
  double * min_x, double * min_y, double * max_x, double * max_y)
{
  if (!enabled_) {
    return;
  }

  std::lock_guard<std::mutex> lock(obstacles_mutex_);

  // Expand bounds to include all obstacles with inflation
  for (const auto& obstacle : obstacles_) {
    // Predict future position based on velocity
    double pred_x = obstacle.pose.position.x + 
                    obstacle.velocity.linear.x * prediction_time_;
    double pred_y = obstacle.pose.position.y + 
                    obstacle.velocity.linear.y * prediction_time_;

    double inflate = inflation_radius_ + obstacle.radius;

    *min_x = std::min(*min_x, pred_x - inflate);
    *min_y = std::min(*min_y, pred_y - inflate);
    *max_x = std::max(*max_x, pred_x + inflate);
    *max_y = std::max(*max_y, pred_y + inflate);
  }
}

void DynamicObstacleLayer::updateCosts(
  nav2_costmap_2d::Costmap2D & master_grid,
  int min_i, int min_j, int max_i, int max_j)
{
  if (!enabled_) {
    return;
  }

  std::lock_guard<std::mutex> lock(obstacles_mutex_);

  // Inflate each obstacle into the costmap
  for (const auto& obstacle : obstacles_) {
    inflateObstacle(master_grid, obstacle, prediction_time_);
  }
}

void DynamicObstacleLayer::inflateObstacle(
  nav2_costmap_2d::Costmap2D & master_grid,
  const DynamicObstacle& obstacle,
  double prediction_time)
{
  // Predict future position
  double pred_x = obstacle.pose.position.x + 
                  obstacle.velocity.linear.x * prediction_time;
  double pred_y = obstacle.pose.position.y + 
                  obstacle.velocity.linear.y * prediction_time;

  // Convert to map coordinates
  unsigned int mx, my;
  if (!master_grid.worldToMap(pred_x, pred_y, mx, my)) {
    return;
  }

  // Calculate inflation radius in cells
  double total_radius = inflation_radius_ + obstacle.radius;
  unsigned int cell_radius = static_cast<unsigned int>(
    total_radius / master_grid.getResolution());

  // Inflate in a circular pattern
  for (int dy = -static_cast<int>(cell_radius); 
       dy <= static_cast<int>(cell_radius); ++dy) {
    for (int dx = -static_cast<int>(cell_radius); 
         dx <= static_cast<int>(cell_radius); ++dx) {
      
      unsigned int cx = mx + dx;
      unsigned int cy = my + dy;

      if (cx >= master_grid.getSizeInCellsX() || 
          cy >= master_grid.getSizeInCellsY()) {
        continue;
      }

      double dist = std::sqrt(dx * dx + dy * dy) * master_grid.getResolution();
      
      if (dist <= total_radius) {
        // Cost decreases linearly with distance
        unsigned char cost = nav2_costmap_2d::LETHAL_OBSTACLE;
        if (dist > obstacle.radius) {
          double ratio = (total_radius - dist) / inflation_radius_;
          cost = static_cast<unsigned char>(
            nav2_costmap_2d::LETHAL_OBSTACLE * ratio);
        }

        unsigned char old_cost = master_grid.getCost(cx, cy);
        if (old_cost == nav2_costmap_2d::NO_INFORMATION) {
          master_grid.setCost(cx, cy, cost);
        } else {
          master_grid.setCost(cx, cy, std::max(old_cost, cost));
        }
      }
    }
  }
}

void DynamicObstacleLayer::reset()
{
  std::lock_guard<std::mutex> lock(obstacles_mutex_);
  obstacles_.clear();
}

void DynamicObstacleLayer::activate()
{
  enabled_ = true;
}

void DynamicObstacleLayer::deactivate()
{
  enabled_ = false;
}

} // namespace swarm_nav_navigation

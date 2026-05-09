// dynamic_obstacle_layer.hpp
// Custom Nav2 costmap layer for dynamic obstacle avoidance

#ifndef SWARM_NAV_NAVIGATION__DYNAMIC_OBSTACLE_LAYER_HPP_
#define SWARM_NAV_NAVIGATION__DYNAMIC_OBSTACLE_LAYER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <nav2_costmap_2d/layer.hpp>
#include <nav2_costmap_2d/layered_costmap.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/twist.hpp>

#include <vector>
#include <string>
#include <mutex>

#include "swarm_nav_msgs/msg/obstacle_array.hpp"

namespace swarm_nav_navigation
{

struct DynamicObstacle
{
  std::string id;
  geometry_msgs::msg::Pose pose;
  geometry_msgs::msg::Twist velocity;
  float radius;
  uint8_t classification;
  rclcpp::Time last_seen;
};

class DynamicObstacleLayer : public nav2_costmap_2d::Layer
{
public:
  DynamicObstacleLayer();
  virtual ~DynamicObstacleLayer();

  virtual void onInitialize() override;
  virtual void updateBounds(
    double robot_x, double robot_y, double robot_yaw,
    double * min_x, double * min_y, double * max_x, double * max_y) override;
  virtual void updateCosts(
    nav2_costmap_2d::Costmap2D & master_grid,
    int min_i, int min_j, int max_i, int max_j) override;

  virtual void reset() override;
  virtual void activate() override;
  virtual void deactivate() override;

private:
  void obstacleCallback(const std::shared_ptr<rclcpp::SerializedMessage> msg);
  
  void inflateObstacle(
    nav2_costmap_2d::Costmap2D & master_grid,
    const DynamicObstacle& obstacle,
    double prediction_time,
    double decay_factor);
  
  void inflatePoint(
    nav2_costmap_2d::Costmap2D & master_grid,
    double world_x, double world_y,
    double obstacle_radius,
    double decay_factor);

  rclcpp::Subscription<rclcpp::SerializedMessage>::SharedPtr obstacle_sub_;
  
  std::vector<DynamicObstacle> obstacles_;
  std::mutex obstacles_mutex_;
  
  // Parameters
  double inflation_radius_;
  double prediction_time_;
  double robot_radius_;
  bool enabled_;
};

} // namespace swarm_nav_navigation

#endif // SWARM_NAV_NAVIGATION__DYNAMIC_OBSTACLE_LAYER_HPP_

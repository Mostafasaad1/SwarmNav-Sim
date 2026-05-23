// mission_executor_main.cpp
// Entry point for mission_executor_node

#include <memory>

#include "rclcpp/executors.hpp"
#include "rclcpp/utilities.hpp"
#include "swarm_nav_coordination/mission_executor_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<swarm_nav_coordination::MissionExecutorNode>();
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.spin();

  rclcpp::shutdown();
  return 0;
}

// mission_executor_node.cpp
// Lifecycle-managed node that loads and runs the mission behavior tree.

#include "swarm_nav_coordination/mission_executor_node.hpp"

#include <filesystem>
#include <memory>
#include <string>

#include <ament_index_cpp/get_package_prefix.hpp>
#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>

namespace swarm_nav_coordination
{

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

MissionExecutorNode::MissionExecutorNode(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("mission_executor", options),
  tick_rate_(10.0)
{
  declare_parameter("robot_id", "robot_0");
  declare_parameter("bt_xml_filename", "");
  declare_parameter("tick_rate", 10.0);
  declare_parameter("bt_plugins_path", "");

  // Create a separate rclcpp::Node for BT plugins to use, since LifecycleNode
  // does not inherit from rclcpp::Node in ROS 2 Jazzy.
  bt_node_ = std::make_shared<rclcpp::Node>(
    std::string(get_name()) + "_bt",
    get_node_options());
}

CallbackReturn MissionExecutorNode::on_configure(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_logger(), "Configuring mission executor");

  // Read parameters at configuration time (allows parameter overrides via NodeOptions)
  get_parameter("robot_id", robot_id_);
  get_parameter("bt_xml_filename", bt_xml_filename_);
  get_parameter("tick_rate", tick_rate_);

  if (bt_xml_filename_.empty()) {
    RCLCPP_ERROR(get_logger(), "bt_xml_filename parameter is empty");
    return CallbackReturn::FAILURE;
  }

  if (!std::filesystem::exists(bt_xml_filename_)) {
    RCLCPP_ERROR(get_logger(), "BT XML file not found: %s", bt_xml_filename_.c_str());
    return CallbackReturn::FAILURE;
  }

  // Register BT plugins from shared library
  std::string plugins_path;
  get_parameter("bt_plugins_path", plugins_path);

  if (plugins_path.empty()) {
    try {
      const std::string prefix = ament_index_cpp::get_package_prefix("swarm_nav_coordination");
      plugins_path = prefix + "/lib/libswarm_bt_nodes.so";
    } catch (const std::exception & e) {
      RCLCPP_ERROR(get_logger(), "Failed to find BT plugins path: %s", e.what());
      return CallbackReturn::FAILURE;
    }
  }

  if (!std::filesystem::exists(plugins_path)) {
    RCLCPP_ERROR(get_logger(), "BT plugins library not found: %s", plugins_path.c_str());
    return CallbackReturn::FAILURE;
  }

  try {
    factory_.registerFromPlugin(plugins_path);
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to register BT plugins: %s", e.what());
    return CallbackReturn::FAILURE;
  }

  // Load behavior tree
  try {
    tree_ = std::make_unique<BT::Tree>(factory_.createTreeFromFile(bt_xml_filename_));
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to create tree from file: %s", e.what());
    return CallbackReturn::FAILURE;
  }

  // Populate blackboard with common entries expected by BT nodes
  auto blackboard = tree_->rootBlackboard();
  blackboard->set("node", bt_node_);
  blackboard->set("robot_id", robot_id_);

  RCLCPP_INFO(get_logger(), "Configuration complete");
  return CallbackReturn::SUCCESS;
}

CallbackReturn MissionExecutorNode::on_activate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_logger(), "Activating mission executor");

  if (!tree_) {
    RCLCPP_ERROR(get_logger(), "No behavior tree loaded");
    return CallbackReturn::FAILURE;
  }

  const auto period_ms = static_cast<int64_t>(1000.0 / tick_rate_);
  timer_ = create_wall_timer(
    std::chrono::milliseconds(std::max(period_ms, int64_t(1))),
    std::bind(&MissionExecutorNode::tick_tree, this));

  RCLCPP_INFO(get_logger(), "Activation complete");
  return CallbackReturn::SUCCESS;
}

CallbackReturn MissionExecutorNode::on_deactivate(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_logger(), "Deactivating mission executor");

  timer_.reset();

  if (tree_) {
    tree_->haltTree();
  }

  RCLCPP_INFO(get_logger(), "Deactivation complete");
  return CallbackReturn::SUCCESS;
}

CallbackReturn MissionExecutorNode::on_cleanup(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_logger(), "Cleaning up mission executor");

  tree_.reset();

  RCLCPP_INFO(get_logger(), "Cleanup complete");
  return CallbackReturn::SUCCESS;
}

void MissionExecutorNode::tick_tree()
{
  if (!tree_) {
    return;
  }

  try {
    const auto status = tree_->rootNode()->executeTick();
    RCLCPP_DEBUG(get_logger(), "Tree tick status: %d", static_cast<int>(status));
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Exception during tree tick: %s", e.what());
  }
}

}  // namespace swarm_nav_coordination

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(swarm_nav_coordination::MissionExecutorNode)

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

// mission_executor_node.cpp
// Lifecycle-managed node that loads and runs the mission behavior tree.

#include "swarm_nav_coordination/mission_executor_node.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <thread>

#include <ament_index_cpp/get_package_prefix.hpp>
#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>

// Nav2 BT action nodes (NavigateToPose etc.) expect std::chrono::milliseconds
// in the blackboard and as port defaults (e.g. "10"). BT.CPP has no built-in
// converter for this type, so we register one here before tree creation.
namespace BT
{
template<>
inline std::chrono::milliseconds convertFromString(StringView key)
{
  return std::chrono::milliseconds(std::stoul(std::string(key.data(), key.size())));
}
}  // namespace BT

namespace swarm_nav_coordination
{

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

MissionExecutorNode::MissionExecutorNode(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("mission_executor", options),
  tick_rate_(10.0),
  stop_spin_(false)
{
  declare_parameter("robot_id", "robot_0");
  declare_parameter("bt_xml_filename", "");
  declare_parameter("tick_rate", 10.0);

  // Separate node for BT plugins – LifecycleNode is not an rclcpp::Node.
  // This node's callbacks MUST be spun separately (see start_bt_spin).
  bt_node_ = std::make_shared<rclcpp::Node>(
    std::string(get_name()) + "_bt",
    get_node_options());
}

MissionExecutorNode::~MissionExecutorNode()
{
  stop_bt_spin();
}

// ---------------------------------------------------------------------------
// Lifecycle callbacks
// ---------------------------------------------------------------------------

CallbackReturn MissionExecutorNode::on_configure(const rclcpp_lifecycle::State &)
{
  RCLCPP_INFO(get_logger(), "Configuring mission executor");

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

  // -------------------------------------------------------------------------
  // Register custom swarm BT plugins
  // The library is installed to lib/<pkg>/<lib>.so — NOT lib/<lib>.so
  // -------------------------------------------------------------------------
  std::string swarm_plugins_path;
  try {
    const std::string prefix = ament_index_cpp::get_package_prefix("swarm_nav_coordination");
    swarm_plugins_path = prefix + "/lib/swarm_nav_coordination/libswarm_bt_nodes.so";
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to locate swarm_nav_coordination prefix: %s", e.what());
    return CallbackReturn::FAILURE;
  }

  if (!std::filesystem::exists(swarm_plugins_path)) {
    RCLCPP_ERROR(get_logger(), "Swarm BT plugins not found: %s", swarm_plugins_path.c_str());
    return CallbackReturn::FAILURE;
  }

  try {
    factory_.registerFromPlugin(swarm_plugins_path);
    RCLCPP_INFO(get_logger(), "Registered swarm BT plugins from %s", swarm_plugins_path.c_str());
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to register swarm BT plugins: %s", e.what());
    return CallbackReturn::FAILURE;
  }

  // -------------------------------------------------------------------------
  // Register Nav2 BT plugins used in mission_tree.xml
  // NavigateToPose handles the full action lifecycle (send goal + wait result).
  // -------------------------------------------------------------------------
  const std::string nav2_lib_dir = "/opt/ros/jazzy/lib/";
  const std::vector<std::string> nav2_plugins = {
    nav2_lib_dir + "libnav2_navigate_to_pose_action_bt_node.so",
    nav2_lib_dir + "libnav2_pipeline_sequence_bt_node.so",
    nav2_lib_dir + "libnav2_recovery_node_bt_node.so",
  };

  for (const auto & plugin_path : nav2_plugins) {
    if (!std::filesystem::exists(plugin_path)) {
      RCLCPP_WARN(get_logger(), "Nav2 BT plugin not found, skipping: %s", plugin_path.c_str());
      continue;
    }
    try {
      factory_.registerFromPlugin(plugin_path);
      RCLCPP_INFO(get_logger(), "Registered Nav2 BT plugin: %s", plugin_path.c_str());
    } catch (const std::exception & e) {
      RCLCPP_WARN(get_logger(), "Could not register Nav2 plugin %s: %s",
        plugin_path.c_str(), e.what());
    }
  }

  // -------------------------------------------------------------------------
  // Load behavior tree from XML
  // BT node constructors access blackboard keys ("node", "robot_id") during
  // createTreeFromFile(), so we must pre-populate the blackboard first and
  // pass it in — not set those keys after the fact.
  // -------------------------------------------------------------------------
  auto blackboard = BT::Blackboard::create();
  blackboard->set("node", bt_node_);
  blackboard->set("robot_id", robot_id_);
  // Nav2 BT action nodes (NavigateToPose, etc.) read these keys from the
  // blackboard during tree construction — seed them with bt_navigator defaults.
  blackboard->set<std::chrono::milliseconds>("bt_loop_duration",
    std::chrono::milliseconds(10));
  blackboard->set<std::chrono::milliseconds>("server_timeout",
    std::chrono::milliseconds(20000));
  // BtActionNode constructor also requires wait_for_service_timeout
  blackboard->set<std::chrono::milliseconds>("wait_for_service_timeout",
    std::chrono::milliseconds(1000));
  // Pre-seed recovery counter used by Nav2 BT nodes during on_success()
  blackboard->set<int>("number_recoveries", 0);

  try {
    tree_ = std::make_unique<BT::Tree>(factory_.createTreeFromFile(bt_xml_filename_, blackboard));
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to create tree from %s: %s",
      bt_xml_filename_.c_str(), e.what());
    return CallbackReturn::FAILURE;
  }

  // Start spinning bt_node_ so subscription + action callbacks fire
  start_bt_spin();

  RCLCPP_INFO(get_logger(), "Configuration complete – robot_id=%s", robot_id_.c_str());
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

  RCLCPP_INFO(get_logger(), "Activation complete – ticking at %.1f Hz", tick_rate_);
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

  stop_bt_spin();
  tree_.reset();

  RCLCPP_INFO(get_logger(), "Cleanup complete");
  return CallbackReturn::SUCCESS;
}

// ---------------------------------------------------------------------------
// BT tick
// ---------------------------------------------------------------------------

void MissionExecutorNode::tick_tree()
{
  if (!tree_) {
    return;
  }

  try {
    const auto status = tree_->rootNode()->executeTick();
    if (status == BT::NodeStatus::SUCCESS) {
      RCLCPP_INFO(get_logger(), "Mission tree completed with SUCCESS");
      timer_.reset();  // Stop ticking – mission done
    } else if (status == BT::NodeStatus::FAILURE) {
      RCLCPP_WARN(get_logger(), "Mission tree returned FAILURE – will retry next tick");
    }
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Exception during tree tick: %s", e.what());
  }
}

// ---------------------------------------------------------------------------
// BT node executor thread management
// ---------------------------------------------------------------------------

void MissionExecutorNode::start_bt_spin()
{
  if (bt_spin_thread_.joinable()) {
    return;  // Already running
  }

  stop_spin_ = false;
  bt_executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  bt_executor_->add_node(bt_node_);

  bt_spin_thread_ = std::thread(
    [this]() {
      RCLCPP_DEBUG(get_logger(), "BT node spin thread started");
      while (!stop_spin_ && rclcpp::ok()) {
        bt_executor_->spin_some(std::chrono::milliseconds(10));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      RCLCPP_DEBUG(get_logger(), "BT node spin thread stopped");
    });
}

void MissionExecutorNode::stop_bt_spin()
{
  stop_spin_ = true;
  if (bt_spin_thread_.joinable()) {
    bt_spin_thread_.join();
  }
  if (bt_executor_) {
    bt_executor_->remove_node(bt_node_);
    bt_executor_.reset();
  }
}

}  // namespace swarm_nav_coordination

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(swarm_nav_coordination::MissionExecutorNode)

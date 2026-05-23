#ifndef SWARM_NAV_COORDINATION__MISSION_EXECUTOR_NODE_HPP_
#define SWARM_NAV_COORDINATION__MISSION_EXECUTOR_NODE_HPP_

#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp>
#include <behaviortree_cpp/bt_factory.h>

namespace swarm_nav_coordination
{

class MissionExecutorNode : public rclcpp_lifecycle::LifecycleNode
{
public:
  explicit MissionExecutorNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  std::string robot_id_;
  std::string bt_xml_filename_;
  double tick_rate_;
  BT::BehaviorTreeFactory factory_;
  std::unique_ptr<BT::Tree> tree_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Node::SharedPtr bt_node_;

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;

  void tick_tree();
};

}  // namespace swarm_nav_coordination

#endif  // SWARM_NAV_COORDINATION__MISSION_EXECUTOR_NODE_HPP_

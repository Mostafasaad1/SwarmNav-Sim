#ifndef SWARM_NAV_NAVIGATION__NEIGHBOR_STATE_AGGREGATOR_NODE_HPP_
#define SWARM_NAV_NAVIGATION__NEIGHBOR_STATE_AGGREGATOR_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include "swarm_nav_msgs/msg/neighbor_state.hpp"
#include "swarm_nav_msgs/msg/neighbor_state_array.hpp"

#include <string>
#include <map>
#include <mutex>
#include <chrono>

namespace swarm_nav_navigation
{

class NeighborStateAggregatorNode : public rclcpp::Node
{
public:
  NeighborStateAggregatorNode();

  void stateCallback(swarm_nav_msgs::msg::NeighborState::SharedPtr msg);
  void publishArray();

  std::map<std::string, swarm_nav_msgs::msg::NeighborState> getStates() const { return states_; }
  double getStateTimeout() const { return state_timeout_; }

private:
  double state_timeout_;

  std::map<std::string, swarm_nav_msgs::msg::NeighborState> states_;
  std::map<std::string, rclcpp::Time> timestamps_;
  std::mutex mutex_;

  rclcpp::Subscription<swarm_nav_msgs::msg::NeighborState>::SharedPtr state_sub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::NeighborStateArray>::SharedPtr array_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

} // namespace swarm_nav_navigation

#endif // SWARM_NAV_NAVIGATION__NEIGHBOR_STATE_AGGREGATOR_NODE_HPP_

// run_auction_bt.cpp
// BehaviorTree action node to run Vickrey auction

#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>
#include "swarm_nav_msgs/msg/auction_result.hpp"

namespace swarm_nav_coordination
{

class RunAuctionBT : public BT::StatefulActionNode
{
public:
  RunAuctionBT(const std::string& name, const BT::NodeConfiguration& config)
    : BT::StatefulActionNode(name, config), received_result_(false)
  {
    node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");
    
    // Get robot_id from blackboard, fallback to "robot_0"
    std::string robot_id = "robot_0";
    config.blackboard->get<std::string>("robot_id", robot_id);
    robot_id_ = robot_id;
    
    // Subscribe to auction results
    result_sub_ = node_->create_subscription<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result",
      rclcpp::QoS(10),
      [this](swarm_nav_msgs::msg::AuctionResult::SharedPtr msg) {
        if (msg->winner_id == robot_id_) {
          assigned_frontier_ = msg->frontier_id;
          received_result_ = true;
          RCLCPP_INFO(node_->get_logger(), "Won auction for frontier: %s", 
                      assigned_frontier_.c_str());
        }
      }
    );
  }

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<int>("frontier_count", "Number of frontiers to auction"),
      BT::OutputPort<std::string>("assigned_frontier", "Frontier assigned to this robot")
    };
  }

  BT::NodeStatus onStart() override
  {
    int frontier_count = 0;
    getInput("frontier_count", frontier_count);
    
    RCLCPP_INFO(node_->get_logger(), "Starting auction for %d frontiers", frontier_count);
    
    // Reset state
    received_result_ = false;
    assigned_frontier_.clear();
    
    // Auction is triggered by auctioneer node automatically when frontiers are detected
    auction_start_time_ = node_->now();
    
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    // Check if we received an auction result
    if (received_result_) {
      setOutput("assigned_frontier", assigned_frontier_);
      RCLCPP_INFO(node_->get_logger(), "Auction complete. Assigned: %s", 
                  assigned_frontier_.c_str());
      return BT::NodeStatus::SUCCESS;
    }
    
    // Check for timeout
    auto elapsed = node_->now() - auction_start_time_;
    if (elapsed.seconds() > 5.0) {  // 5 second timeout
      RCLCPP_WARN(node_->get_logger(), "Auction timeout - no frontier assigned");
      return BT::NodeStatus::FAILURE;
    }
    
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    RCLCPP_WARN(node_->get_logger(), "Auction halted");
  }

private:
  rclcpp::Node::SharedPtr node_;
  std::string robot_id_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_sub_;
  rclcpp::Time auction_start_time_;
  std::string assigned_frontier_;
  bool received_result_;
};

} // namespace swarm_nav_coordination

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<swarm_nav_coordination::RunAuctionBT>("RunAuctionBT");
}

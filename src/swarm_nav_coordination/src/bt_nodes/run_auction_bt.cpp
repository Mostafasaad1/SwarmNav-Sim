// run_auction_bt.cpp
// BehaviorTree action node to run Vickrey auction

#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <behaviortree_cpp_v3/bt_factory.h>

namespace swarm_nav_coordination
{

class RunAuctionBT : public BT::StatefulActionNode
{
public:
  RunAuctionBT(const std::string& name, const BT::NodeConfiguration& config)
    : BT::StatefulActionNode(name, config)
  {
    node_ = config.blackboard->get<rclcpp::Node::SharedPtr>("node");
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
    
    // TODO: Trigger auction announce
    auction_start_time_ = node_->now();
    
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    // Check if auction has completed (timeout or result received)
    auto elapsed = node_->now() - auction_start_time_;
    
    if (elapsed.seconds() > 2.0) {  // 2 second timeout
      // TODO: Check if we won any frontier
      
      // Placeholder: simulate winning a frontier
      std::string assigned_frontier = "frontier_1";
      setOutput("assigned_frontier", assigned_frontier);
      
      RCLCPP_INFO(node_->get_logger(), "Auction complete. Assigned: %s", 
                  assigned_frontier.c_str());
      return BT::NodeStatus::SUCCESS;
    }
    
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    RCLCPP_WARN(node_->get_logger(), "Auction halted");
  }

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Time auction_start_time_;
};

} // namespace swarm_nav_coordination

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<swarm_nav_coordination::RunAuctionBT>("RunAuctionBT");
}

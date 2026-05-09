// auctioneer_node.cpp
// Implements Vickrey auction for decentralized task allocation

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>
#include <nav_msgs/msg/odometry.hpp>

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>

#include "swarm_nav_msgs/msg/frontier_array.hpp"
#include "swarm_nav_msgs/msg/auction_announce.hpp"
#include "swarm_nav_msgs/msg/auction_bid.hpp"
#include "swarm_nav_msgs/msg/auction_result.hpp"

namespace swarm_nav_coordination
{

enum AuctionState
{
  STATE_IDLE,
  STATE_ANNOUNCE,
  STATE_COLLECT_BIDS,
  STATE_RESOLVE
};

struct Bid
{
  std::string robot_id;
  std::string frontier_id;
  float cost;
};

class AuctioneerNode : public rclcpp::Node
{
public:
  AuctioneerNode() : Node("auctioneer_node"), state_(STATE_IDLE)
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("bid_timeout_ms", 500);
    this->declare_parameter("use_sim_time", true);
    
    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    bid_timeout_ms_ = this->get_parameter("bid_timeout_ms").as_int();
    
    RCLCPP_INFO(this->get_logger(), "Auctioneer initialized for %s", robot_id_.c_str());
    RCLCPP_INFO(this->get_logger(), "Bid timeout: %d ms", bid_timeout_ms_);
    
    // Subscribe to own odometry for position
    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "odom", 10,
      std::bind(&AuctioneerNode::odomCallback, this, std::placeholders::_1)
    );
    
    // Subscribers
    frontier_sub_ = this->create_subscription<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers", 10,
      std::bind(&AuctioneerNode::frontierCallback, this, std::placeholders::_1)
    );
    
    auction_announce_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionAnnounce>(
      "/swarm/auction/announce", 10,
      std::bind(&AuctioneerNode::auctionAnnounceCallback, this, std::placeholders::_1)
    );
    
    bid_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionBid>(
      "/swarm/auction/bid", 10,
      std::bind(&AuctioneerNode::bidCallback, this, std::placeholders::_1)
    );
    
    result_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result", 10,
      std::bind(&AuctioneerNode::resultCallback, this, std::placeholders::_1)
    );
    
    // Publishers
    announce_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionAnnounce>(
      "/swarm/auction/announce", 10
    );
    
    bid_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionBid>(
      "/swarm/auction/bid", 10
    );
    
    result_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result", 10
    );
    
    // Timer for auction state machine
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100),
      std::bind(&AuctioneerNode::timerCallback, this)
    );
    
    RCLCPP_INFO(this->get_logger(), "Auctioneer ready");
  }

private:
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    current_pose_ = msg->pose.pose;
  }
  
  void frontierCallback(const swarm_nav_msgs::msg::FrontierArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store frontiers and trigger auction if we're idle
    if (state_ == STATE_IDLE && !msg->frontiers.empty()) {
      current_frontiers_ = msg->frontiers;
      state_ = STATE_ANNOUNCE;
      RCLCPP_INFO(this->get_logger(), "Received %zu frontiers, starting auction", 
                  current_frontiers_.size());
    }
  }
  
  void auctionAnnounceCallback(const swarm_nav_msgs::msg::AuctionAnnounce::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Don't bid on our own auctions
    if (msg->auctioneer_id == robot_id_) {
      return;
    }
    
    RCLCPP_INFO(this->get_logger(), "Received auction from %s with %zu frontiers",
                msg->auctioneer_id.c_str(), msg->frontiers.size());
    
    // Calculate bids for each frontier
    for (const auto& frontier : msg->frontiers) {
      float cost = calculateBidCost(frontier.centroid);
      
      swarm_nav_msgs::msg::AuctionBid bid_msg;
      bid_msg.auction_id = msg->auction_id;
      bid_msg.frontier_id = frontier.frontier_id;
      bid_msg.robot_id = robot_id_;
      bid_msg.bid_cost = cost;
      
      bid_pub_->publish(bid_msg);
      
      RCLCPP_DEBUG(this->get_logger(), "Submitted bid %.2f for frontier %s",
                   cost, frontier.frontier_id.c_str());
    }
  }
  
  void bidCallback(const swarm_nav_msgs::msg::AuctionBid::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Only collect bids for our current auction
    if (msg->auction_id != current_auction_id_) {
      return;
    }
    
    // Store the bid
    Bid bid;
    bid.robot_id = msg->robot_id;
    bid.frontier_id = msg->frontier_id;
    bid.cost = msg->bid_cost;
    
    received_bids_.push_back(bid);
    
    RCLCPP_DEBUG(this->get_logger(), "Received bid from %s: %.2f for %s",
                 msg->robot_id.c_str(), msg->bid_cost, msg->frontier_id.c_str());
  }
  
  void resultCallback(const swarm_nav_msgs::msg::AuctionResult::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (msg->winner_id == robot_id_) {
      RCLCPP_INFO(this->get_logger(), "Won frontier %s with cost %.2f",
                  msg->frontier_id.c_str(), msg->winning_bid);
      // TODO: Navigate to this frontier
    }
  }
  
  void timerCallback()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    switch (state_) {
      case STATE_IDLE:
        // Wait for new frontiers
        break;
        
      case STATE_ANNOUNCE:
        announceAuction();
        state_ = STATE_COLLECT_BIDS;
        bid_deadline_ = this->now() + rclcpp::Duration::from_nanoseconds(bid_timeout_ms_ * 1000000);
        break;
        
      case STATE_COLLECT_BIDS:
        if (this->now() >= bid_deadline_) {
          state_ = STATE_RESOLVE;
        }
        break;
        
      case STATE_RESOLVE:
        resolveAuction();
        state_ = STATE_IDLE;
        break;
    }
  }
  
  void announceAuction()
  {
    RCLCPP_INFO(this->get_logger(), "Announcing auction for %zu frontiers", 
                current_frontiers_.size());
    
    // Generate unique auction ID
    current_auction_id_ = generateAuctionId();
    
    // Publish AuctionAnnounce message with current frontiers
    swarm_nav_msgs::msg::AuctionAnnounce announce_msg;
    announce_msg.auction_id = current_auction_id_;
    announce_msg.auctioneer_id = robot_id_;
    announce_msg.frontiers = current_frontiers_;
    
    announce_pub_->publish(announce_msg);
    
    // Clear previous bids
    received_bids_.clear();
  }
  
  void resolveAuction()
  {
    RCLCPP_INFO(this->get_logger(), "Resolving auction with %zu bids", received_bids_.size());
    
    // Group bids by frontier
    std::map<std::string, std::vector<Bid>> bids_by_frontier;
    for (const auto& bid : received_bids_) {
      bids_by_frontier[bid.frontier_id].push_back(bid);
    }
    
    // Apply Vickrey auction (second-price sealed-bid) for each frontier
    for (const auto& [frontier_id, bids] : bids_by_frontier) {
      if (bids.empty()) continue;
      
      // Find lowest cost bid (winner)
      auto winner_it = std::min_element(bids.begin(), bids.end(),
        [](const Bid& a, const Bid& b) { return a.cost < b.cost; });
      
      // Find second-lowest cost (price to pay)
      float second_price = winner_it->cost;
      if (bids.size() > 1) {
        auto second_it = std::min_element(bids.begin(), bids.end(),
          [&winner_it](const Bid& a, const Bid& b) {
            if (&a == &(*winner_it)) return false;
            if (&b == &(*winner_it)) return true;
            return a.cost < b.cost;
          });
        second_price = second_it->cost;
      }
      
      RCLCPP_INFO(this->get_logger(), 
                  "Frontier %s awarded to %s (bid: %.2f, pays: %.2f)",
                  frontier_id.c_str(), winner_it->robot_id.c_str(),
                  winner_it->cost, second_price);
      
      // Publish AuctionResult message
      publishResult(frontier_id, winner_it->robot_id, second_price);
    }
  }
  
  void publishResult(const std::string& frontier_id, 
                    const std::string& winner_id, 
                    float winning_bid)
  {
    swarm_nav_msgs::msg::AuctionResult result_msg;
    result_msg.auction_id = current_auction_id_;
    result_msg.frontier_id = frontier_id;
    result_msg.winner_id = winner_id;
    result_msg.winning_bid = winning_bid;
    
    result_pub_->publish(result_msg);
    
    RCLCPP_INFO(this->get_logger(), "Published result: %s -> %s (cost: %.2f)", 
                frontier_id.c_str(), winner_id.c_str(), winning_bid);
  }
  
  float calculateBidCost(const geometry_msgs::msg::Point& frontier_centroid)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Calculate Euclidean distance from current pose to frontier
    double dx = frontier_centroid.x - current_pose_.position.x;
    double dy = frontier_centroid.y - current_pose_.position.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    // Base cost is distance
    float cost = static_cast<float>(distance);
    
    // Add simulated workload modifier (0-20% increase)
    float workload_modifier = 1.0f + (rand() % 20) / 100.0f;
    cost *= workload_modifier;
    
    return cost;
  }
  
  std::string generateAuctionId()
  {
    static int counter = 0;
    return robot_id_ + "_auction_" + std::to_string(counter++);
  }

  // Member variables
  std::string robot_id_;
  int bid_timeout_ms_;
  
  AuctionState state_;
  std::string current_auction_id_;
  std::vector<swarm_nav_msgs::msg::Frontier> current_frontiers_;
  std::vector<Bid> received_bids_;
  rclcpp::Time bid_deadline_;
  geometry_msgs::msg::Pose current_pose_;
  
  std::mutex mutex_;
  rclcpp::TimerBase::SharedPtr timer_;
  
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionAnnounce>::SharedPtr auction_announce_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionBid>::SharedPtr bid_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_sub_;
  
  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionAnnounce>::SharedPtr announce_pub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionBid>::SharedPtr bid_pub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_pub_;
};

} // namespace swarm_nav_coordination

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_coordination::AuctioneerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

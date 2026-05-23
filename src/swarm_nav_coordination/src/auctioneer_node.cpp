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

// auctioneer_node.cpp
// Implements Vickrey auction for decentralized task allocation

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <random>
#include <functional>

#include "swarm_nav_msgs/msg/frontier_array.hpp"
#include "swarm_nav_msgs/msg/auction_announce.hpp"
#include "swarm_nav_msgs/msg/auction_bid.hpp"
#include "swarm_nav_msgs/msg/auction_result.hpp"
#include "swarm_nav_msgs/msg/neighbor_state_array.hpp"

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
  AuctioneerNode()
  : Node("auctioneer_node"), state_(STATE_IDLE)
  {
    // Declare parameters
    this->declare_parameter("robot_id", "robot_0");
    this->declare_parameter("bid_timeout_ms", 500);
    this->declare_parameter("nominal_speed", 0.5);
    if (!this->has_parameter("use_sim_time")) {this->declare_parameter("use_sim_time", true);}

    // Get parameters
    robot_id_ = this->get_parameter("robot_id").as_string();
    bid_timeout_ms_ = this->get_parameter("bid_timeout_ms").as_int();
    nominal_speed_ = this->get_parameter("nominal_speed").as_double();

    // Seed deterministic RNG from hash of robot_id
    std::hash<std::string> hasher;
    rng_.seed(static_cast<uint32_t>(hasher(robot_id_)));

    RCLCPP_INFO(this->get_logger(), "Auctioneer initialized for %s", robot_id_.c_str());
    RCLCPP_INFO(this->get_logger(), "Bid timeout: %d ms", bid_timeout_ms_);

    // Initialize TF buffer and listener
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // Subscribers
    frontier_sub_ = this->create_subscription<swarm_nav_msgs::msg::FrontierArray>(
      "frontiers", 10,
      [this](swarm_nav_msgs::msg::FrontierArray::SharedPtr msg) {this->frontierCallback(msg);});

    auction_announce_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionAnnounce>(
      "/swarm/auction/announce", rclcpp::QoS(1).best_effort(),
      [this](swarm_nav_msgs::msg::AuctionAnnounce::SharedPtr msg) {
        this->auctionAnnounceCallback(msg);
      });

    bid_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionBid>(
      "/swarm/auction/bid", 10,
      [this](swarm_nav_msgs::msg::AuctionBid::SharedPtr msg) {this->bidCallback(msg);});

    result_sub_ = this->create_subscription<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result", 10,
      [this](swarm_nav_msgs::msg::AuctionResult::SharedPtr msg) {this->resultCallback(msg);});

    // Subscribe to neighbor states for obstacle density estimation
    neighbor_sub_ = this->create_subscription<swarm_nav_msgs::msg::NeighborStateArray>(
      "/swarm/neighbor_states",
      rclcpp::SensorDataQoS(),
      [this](swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg) {
        this->neighborCallback(msg);
      });

    // Publishers
    announce_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionAnnounce>(
      "/swarm/auction/announce", rclcpp::QoS(1).best_effort()
    );

    bid_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionBid>(
      "/swarm/auction/bid", rclcpp::QoS(10).reliable()
    );

    result_pub_ = this->create_publisher<swarm_nav_msgs::msg::AuctionResult>(
      "/swarm/auction/result", 10
    );

    // Timer for auction state machine
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(100),
      [this]() {this->timerCallback();});

    RCLCPP_INFO(this->get_logger(), "Auctioneer ready");
  }

private:
  void frontierCallback(const swarm_nav_msgs::msg::FrontierArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Store frontiers and trigger auction if we're idle
    if (state_ == STATE_IDLE && !msg->frontiers.empty()) {
      current_frontiers_ = msg->frontiers;
      state_ = STATE_ANNOUNCE;
      RCLCPP_INFO(
        this->get_logger(), "Received %zu frontiers, starting auction",
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

    // Check if our currently assigned frontier is still active in the announced frontiers
    bool assigned_still_active = false;
    for (const auto & frontier : msg->frontiers) {
      if (frontier.frontier_id == current_assigned_frontier_) {
        assigned_still_active = true;
        break;
      }
    }
    if (!assigned_still_active) {
      current_assigned_frontier_ = "";
    }

    RCLCPP_INFO(
      this->get_logger(), "Received auction from %s with %zu frontiers",
      msg->auctioneer_id.c_str(), msg->frontiers.size());

    // Calculate bids for each frontier
    for (const auto & frontier : msg->frontiers) {
      float cost = calculateBidCost(frontier.centroid, frontier.frontier_id, msg->auctioneer_id);

      swarm_nav_msgs::msg::AuctionBid bid_msg;
      bid_msg.auction_id = msg->auction_id;
      bid_msg.frontier_id = frontier.frontier_id;
      bid_msg.robot_id = robot_id_;
      bid_msg.bid_cost = cost;

      bid_pub_->publish(bid_msg);

      RCLCPP_DEBUG(
        this->get_logger(), "Submitted bid %.2f for frontier %s",
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

    RCLCPP_DEBUG(
      this->get_logger(), "Received bid from %s: %.2f for %s",
      msg->robot_id.c_str(), msg->bid_cost, msg->frontier_id.c_str());
  }

  void resultCallback(const swarm_nav_msgs::msg::AuctionResult::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (msg->winner_id == robot_id_) {
      RCLCPP_INFO(
        this->get_logger(), "Won frontier %s with cost %.2f",
        msg->frontier_id.c_str(), msg->winning_bid);
      // Track current assigned frontier for task-switch penalty
      current_assigned_frontier_ = msg->frontier_id;
    }
  }

  void neighborCallback(const swarm_nav_msgs::msg::NeighborStateArray::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    neighbor_poses_.clear();
    for (const auto & neighbor : msg->neighbors) {
      if (neighbor.robot_id != robot_id_) {
        neighbor_poses_[neighbor.robot_id] = neighbor.pose;
      }
    }
  }

  void timerCallback()
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Update current pose from TF
    try {
      auto transform = tf_buffer_->lookupTransform(
        "map",
        robot_id_ + "/base_footprint",
        tf2::TimePointZero
      );
      current_pose_.position.x = transform.transform.translation.x;
      current_pose_.position.y = transform.transform.translation.y;
      current_pose_.position.z = transform.transform.translation.z;
      current_pose_.orientation = transform.transform.rotation;
    } catch (const tf2::TransformException & ex) {
      RCLCPP_DEBUG(this->get_logger(), "Could not transform to map: %s", ex.what());
    }

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
    RCLCPP_INFO(
      this->get_logger(), "Announcing auction for %zu frontiers",
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

    // Check if our currently assigned frontier is still active in our current frontiers
    bool assigned_still_active = false;
    for (const auto & frontier : current_frontiers_) {
      if (frontier.frontier_id == current_assigned_frontier_) {
        assigned_still_active = true;
        break;
      }
    }
    if (!assigned_still_active) {
      current_assigned_frontier_ = "";
    }

    // Submit self-bids immediately
    for (const auto & frontier : current_frontiers_) {
      float cost = calculateBidCost(frontier.centroid, frontier.frontier_id, robot_id_);
      Bid self_bid;
      self_bid.robot_id = robot_id_;
      self_bid.frontier_id = frontier.frontier_id;
      self_bid.cost = cost;
      received_bids_.push_back(self_bid);

      RCLCPP_INFO(
        this->get_logger(), "Self-bid %.2f for frontier %s",
        cost, frontier.frontier_id.c_str());
    }
  }

  void resolveAuction()
  {
    RCLCPP_INFO(this->get_logger(), "Resolving auction with %zu bids", received_bids_.size());

    // Group bids by frontier
    std::map<std::string, std::vector<Bid>> bids_by_frontier;
    for (const auto & bid : received_bids_) {
      bids_by_frontier[bid.frontier_id].push_back(bid);
    }

    // Apply Vickrey auction (second-price sealed-bid) for each frontier
    for (const auto & [frontier_id, bids] : bids_by_frontier) {
      if (bids.empty()) {continue;}

      // Find lowest cost bid (winner) with tie-breaking by robot_id
      const Bid * winner = nullptr;
      for (const auto & bid : bids) {
        if (!winner) {
          winner = &bid;
        } else {
          if (bid.cost < winner->cost) {
            winner = &bid;
          } else if (bid.cost == winner->cost && bid.robot_id < winner->robot_id) {
            winner = &bid;
          }
        }
      }

      // Find second-lowest cost (price to pay)
      float second_price = winner->cost;
      const Bid * second_winner = nullptr;
      for (const auto & bid : bids) {
        if (&bid == winner) {continue;}
        if (!second_winner) {
          second_winner = &bid;
        } else {
          if (bid.cost < second_winner->cost) {
            second_winner = &bid;
          } else if (bid.cost == second_winner->cost && bid.robot_id < second_winner->robot_id) {
            second_winner = &bid;
          }
        }
      }
      if (second_winner) {
        second_price = second_winner->cost;
      }

      RCLCPP_INFO(
        this->get_logger(),
        "Frontier %s awarded to %s (bid: %.2f, pays: %.2f)",
        frontier_id.c_str(), winner->robot_id.c_str(),
        winner->cost, second_price);

      // Publish AuctionResult message
      publishResult(frontier_id, winner->robot_id, second_price);
    }
  }

  void publishResult(
    const std::string & frontier_id,
    const std::string & winner_id,
    float winning_bid)
  {
    swarm_nav_msgs::msg::AuctionResult result_msg;
    result_msg.auction_id = current_auction_id_;
    result_msg.frontier_id = frontier_id;
    result_msg.winner_id = winner_id;
    result_msg.winning_bid = winning_bid;

    result_pub_->publish(result_msg);

    RCLCPP_INFO(
      this->get_logger(), "Published result: %s -> %s (cost: %.2f)",
      frontier_id.c_str(), winner_id.c_str(), winning_bid);
  }

  float calculateBidCost(
    const geometry_msgs::msg::Point & frontier_centroid,
    const std::string & frontier_id,
    const std::string & auctioneer_id)
  {
    // NOTE: Caller already holds mutex_, do not lock again
    // Spec formula: cost = distance*1.0 + travel_time*0.5 + obstacle_density*2.0 + task_switch*0.3

    geometry_msgs::msg::Point global_centroid = frontier_centroid;
    try {
      auto transform = tf_buffer_->lookupTransform(
        "map",
        auctioneer_id + "/map",
        tf2::TimePointZero
      );
      double yaw = tf2::getYaw(transform.transform.rotation);
      double cx = frontier_centroid.x;
      double cy = frontier_centroid.y;
      global_centroid.x = cx * cos(yaw) - cy * sin(yaw) + transform.transform.translation.x;
      global_centroid.y = cx * sin(yaw) + cy * cos(yaw) + transform.transform.translation.y;
    } catch (const tf2::TransformException & ex) {
      RCLCPP_DEBUG(this->get_logger(), "Could not transform frontier to map: %s", ex.what());
      // High cost if we can't transform
      return 1000.0f;
    }

    // 1. Euclidean distance from current pose to frontier
    double dx = global_centroid.x - current_pose_.position.x;
    double dy = global_centroid.y - current_pose_.position.y;
    double distance = std::sqrt(dx * dx + dy * dy);

    // 2. Travel time at nominal speed
    double travel_time = distance / nominal_speed_;

    // 3. Obstacle density: fraction of known neighbors within 5m of frontier
    //    Uses a Gaussian-weighted approximation without costmap access.
    //    Count neighbor robots within 5m of the frontier centroid as proxy for congestion.
    float obstacle_density = 0.0f;
    {
      const double density_radius = 5.0;
      int nearby_count = 0;
      for (const auto & [id, pose] : neighbor_poses_) {
        double ndx = pose.position.x - frontier_centroid.x;
        double ndy = pose.position.y - frontier_centroid.y;
        if (std::sqrt(ndx * ndx + ndy * ndy) < density_radius) {
          nearby_count++;
        }
      }
      // Normalize: 0 neighbors = 0.0 density, 5+ neighbors = 1.0
      obstacle_density = std::min(1.0f, nearby_count / 5.0f);
    }

    // 4. Task switch penalty: 1.0 if already assigned a different active frontier
    float task_switch_penalty = 0.0f;
    if (!current_assigned_frontier_.empty() &&
      current_assigned_frontier_ != frontier_id)
    {
      task_switch_penalty = 1.0f;
    }

    // Apply spec formula
    float cost = static_cast<float>(
      distance * 1.0 + travel_time * 0.5 +
      obstacle_density * 2.0 + task_switch_penalty * 0.3);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Bid cost: dist=%.2f tt=%.2f density=%.2f switch=%.2f -> %.2f",
      distance, travel_time, (double)obstacle_density,
      (double)task_switch_penalty, cost);

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
  double nominal_speed_;

  AuctionState state_;
  std::string current_auction_id_;
  std::string current_assigned_frontier_;
  std::vector<swarm_nav_msgs::msg::Frontier> current_frontiers_;
  std::vector<Bid> received_bids_;
  rclcpp::Time bid_deadline_;
  geometry_msgs::msg::Pose current_pose_;
  std::map<std::string, geometry_msgs::msg::Pose> neighbor_poses_;  // for density estimation

  std::mt19937 rng_;
  std::mutex mutex_;
  rclcpp::TimerBase::SharedPtr timer_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  rclcpp::Subscription<swarm_nav_msgs::msg::FrontierArray>::SharedPtr frontier_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionAnnounce>::SharedPtr auction_announce_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionBid>::SharedPtr bid_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_sub_;
  rclcpp::Subscription<swarm_nav_msgs::msg::NeighborStateArray>::SharedPtr neighbor_sub_;

  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionAnnounce>::SharedPtr announce_pub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionBid>::SharedPtr bid_pub_;
  rclcpp::Publisher<swarm_nav_msgs::msg::AuctionResult>::SharedPtr result_pub_;
};

}  // namespace swarm_nav_coordination

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<swarm_nav_coordination::AuctioneerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

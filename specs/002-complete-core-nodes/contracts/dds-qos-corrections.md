# DDS Topic QoS Corrections

**Extends**: `specs/001-swarm-nav-sim/contracts/dds-topics.md` — no topic additions; only QoS profile enforcement.

## Required QoS Changes

These corrections align the code with the contract defined in `001-swarm-nav-sim`:

| Topic | Required QoS | Current Code QoS | Fix Required |
|-------|-------------|-------------------|--------------|
| `/swarm/auction/announce` | BestEffort, KeepLast(1) | `rclcpp::QoS(10)` (default Reliable) | Yes — change to `rclcpp::QoS(1).best_effort()` |
| `/swarm/auction/bid` | Reliable, Volatile | `rclcpp::QoS(10)` (default) | Yes — change to `rclcpp::QoS(10).reliable()` (volatile is default) |
| `/swarm/auction/result` | Reliable, KeepLast(10) | `rclcpp::QoS(10)` (default) | Correct — no change needed |
| `/swarm/tracked_obstacles` | SensorData | `rclcpp::SensorDataQoS()` | Correct — no change needed |
| `/swarm/neighbor_states` | SensorData | `rclcpp::QoS(10)` (default) | Yes — change to `rclcpp::SensorDataQoS()` |
| `/swarm/global_map` | TransientLocal | `rclcpp::QoS(10).transient_local()` | Correct — no change needed |
| `frontiers` (per-robot) | Default | `rclcpp::QoS(10)` | Correct — no change needed |

## New Topic: Individual NeighborState

| Topic | Message Type | QoS Profile | Publisher | Subscriber |
|-------|-------------|-------------|-----------|------------|
| `/swarm/robot_state` | `swarm_nav_msgs/NeighborState` | SensorData | Each robot's ORCA filter | `neighbor_state_aggregator_node` |
| `/swarm/neighbor_states` | `swarm_nav_msgs/NeighborStateArray` | SensorData | `neighbor_state_aggregator_node` | All ORCA filters, `graph_merge_node` |

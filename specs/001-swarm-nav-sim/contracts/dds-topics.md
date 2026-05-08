# DDS Interface Contracts

## Inter-Robot Communication Contracts

This system relies on strictly namespaced and global DDS topics for decentralized communication.

### Global / Swarm Topics (No Namespace)

| Topic | Message Type | QoS Profile | Publisher | Subscriber | Purpose |
|-------|--------------|-------------|-----------|------------|---------|
| `/mrg_slam/shared_graph` | `mrg_slam_msgs/Graph` | Reliable, KeepLast(5) | All Robots (`mrg_slam`) | All Robots (`mrg_slam`) | Peer-to-peer pose graph exchange |
| `/swarm/global_map` | `nav_msgs/OccupancyGrid` | TransientLocal | Merging Robot | Observers / BT | The unified global warehouse map |
| `/swarm/auction/announce` | `swarm_nav_msgs/AuctionAnnounce` | BestEffort, KeepLast(1) | Any Robot | All Robots | Initiating task auction for frontiers |
| `/swarm/auction/bid` | `swarm_nav_msgs/AuctionBid` | Reliable, Volatile | Bidding Robots | Auctioneer Robot | Submitting Vickrey bids |
| `/swarm/auction/result` | `swarm_nav_msgs/AuctionResult` | Reliable, KeepLast(10) | Auctioneer Robot | All Robots | Declaring winning robot for a frontier |
| `/swarm/tracked_obstacles`| `swarm_nav_msgs/ObstacleArray` | SensorData | Ground Truth / Perception | `DynamicObstacleLayer` | Central tracking of NPCs/robots |
| `/swarm/neighbor_states` | `swarm_nav_msgs/NeighborStateArray`| SensorData | All Robots | All Robots (`orca_velocity_filter`) | ORCA collision avoidance states |

### Per-Robot Topics (Namespaced: `robot_X/`)

| Topic | Message Type | QoS Profile | Publisher | Subscriber | Purpose |
|-------|--------------|-------------|-----------|------------|---------|
| `scan` | `sensor_msgs/LaserScan` | SensorData | Sim Plugin | `mrg_slam`, `ObstacleLayer` | LiDAR data |
| `odom` | `nav_msgs/Odometry` | SensorData | EKF / Sim Plugin | `mrg_slam`, Nav2 | Fused odometry |
| `map` | `nav_msgs/OccupancyGrid` | TransientLocal | `mrg_slam` | `frontier_detector`, Nav2 | Local SLAM map |
| `cmd_vel_nav2` | `geometry_msgs/Twist` | Default | TEB Planner | `orca_velocity_filter` | Planner requested velocity |
| `cmd_vel` | `geometry_msgs/Twist` | Default | `orca_velocity_filter` | Sim Plugin (Motors) | Safe velocity command (ORCA filtered) |
| `tf` / `tf_static` | `tf2_msgs/TFMessage` | Default | Various | Various | Local robot TF tree |

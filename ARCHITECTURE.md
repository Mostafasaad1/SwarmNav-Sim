# SwarmNav-Sim System Architecture

Visual overview of the multi-robot swarm navigation system.

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                         SwarmNav-Sim System                          │
│                    Multi-Robot Warehouse Exploration                 │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                          Simulation Layer                            │
├─────────────────────────────────────────────────────────────────────┤
│  Gazebo Fortress                                                     │
│  ├─ Warehouse World (40m x 60m)                                     │
│  ├─ 3-5 Differential Drive Robots                                   │
│  ├─ Lidar Sensors (360° scan)                                       │
│  └─ Dynamic Obstacles (forklifts, humans)                           │
└─────────────────────────────────────────────────────────────────────┘
                              ↓ Topics
┌─────────────────────────────────────────────────────────────────────┐
│                      Per-Robot Components                            │
│                    (Replicated for each robot)                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                    Perception Layer                         │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  /robot_X/scan (LaserScan)                                 │    │
│  │  /robot_X/odom (Odometry)                                  │    │
│  └────────────────────────────────────────────────────────────┘    │
│                              ↓                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                      SLAM Layer                             │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Graph-Based SLAM                                          │    │
│  │  ├─ Loop Closure Detection                                 │    │
│  │  ├─ Pose Graph Optimization                                │    │
│  │  └─ Occupancy Grid Generation                              │    │
│  │                                                             │    │
│  │  Output: /robot_X/map (OccupancyGrid)                      │    │
│  └────────────────────────────────────────────────────────────┘    │
│                              ↓                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                  Coordination Layer                         │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Frontier Detector                                         │    │
│  │  ├─ Detects unexplored boundaries                          │    │
│  │  ├─ Calculates utility (size, distance)                    │    │
│  │  └─ Publishes: /robot_X/frontiers                          │    │
│  │                                                             │    │
│  │  Auctioneer Node                                           │    │
│  │  ├─ Receives frontiers                                     │    │
│  │  ├─ Calculates bid cost                                    │    │
│  │  ├─ Participates in auctions                               │    │
│  │  └─ Publishes: /swarm/auction/*                            │    │
│  └────────────────────────────────────────────────────────────┘    │
│                              ↓                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │                   Navigation Layer                          │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Nav2 Stack                                                │    │
│  │  ├─ Controller Server (DWB)                                │    │
│  │  ├─ Planner Server (NavFn)                                 │    │
│  │  ├─ Behavior Server                                        │    │
│  │  └─ BT Navigator                                           │    │
│  │                                                             │    │
│  │  ORCA Velocity Filter                                      │    │
│  │  ├─ Receives: /robot_X/cmd_vel_nav2                        │    │
│  │  ├─ Applies collision avoidance                            │    │
│  │  └─ Publishes: /robot_X/cmd_vel                            │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                      Swarm-Level Components                          │
│                    (Shared across all robots)                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │            Neighbor State Aggregator                        │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Subscribes: /swarm/robot_state (from each robot)          │    │
│  │  Publishes: /swarm/neighbor_states (NeighborStateArray)    │    │
│  │  Purpose: Centralized state distribution                    │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │              Obstacle Tracker                               │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Tracks dynamic obstacles (forklifts, humans)              │    │
│  │  Publishes: /swarm/tracked_obstacles                        │    │
│  │  Purpose: Shared obstacle awareness                         │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │              Graph Merge Coordinator                        │    │
│  ├────────────────────────────────────────────────────────────┤    │
│  │  Merges SLAM graphs on robot rendezvous                    │    │
│  │  Publishes: /mrg_slam/shared_graph                          │    │
│  │  Purpose: Multi-robot map consistency                       │    │
│  └────────────────────────────────────────────────────────────┘    │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                       Evaluation Layer                               │
├─────────────────────────────────────────────────────────────────────┤
│  Coverage Evaluator                                                  │
│  ├─ Monitors: /robot_X/map                                          │
│  └─ Outputs: coverage_results.json                                  │
│                                                                       │
│  Collision Monitor                                                   │
│  ├─ Monitors: /robot_X/odom                                         │
│  └─ Outputs: collision_results.json                                 │
│                                                                       │
│  SLAM Metrics                                                        │
│  ├─ Monitors: /robot_X/odom, /robot_X/ground_truth                  │
│  └─ Outputs: slam_metrics.json                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow Diagram

```
Sensor Data → SLAM → Map → Frontier Detection → Auction
                                                    ↓
                                              Task Assignment
                                                    ↓
                                            Navigation Planning
                                                    ↓
                                              ORCA Filter
                                                    ↓
                                            Robot Actuation
```

---

## Topic Communication Graph

```
Per-Robot Topics:
  /robot_X/scan              (sensor_msgs/LaserScan)
  /robot_X/odom              (nav_msgs/Odometry)
  /robot_X/map               (nav_msgs/OccupancyGrid)
  /robot_X/frontiers         (swarm_nav_msgs/FrontierArray)
  /robot_X/cmd_vel_nav2      (geometry_msgs/Twist)
  /robot_X/cmd_vel           (geometry_msgs/Twist)

Swarm-Level Topics:
  /swarm/robot_state         (swarm_nav_msgs/NeighborState)
  /swarm/neighbor_states     (swarm_nav_msgs/NeighborStateArray)
  /swarm/tracked_obstacles   (swarm_nav_msgs/ObstacleArray)
  /swarm/auction/announce    (swarm_nav_msgs/AuctionAnnounce)
  /swarm/auction/bid         (swarm_nav_msgs/AuctionBid)
  /swarm/auction/result      (swarm_nav_msgs/AuctionResult)
  /mrg_slam/shared_graph     (swarm_nav_msgs/GraphData)
```

---

## Package Dependencies

```
swarm_nav_msgs (Message Definitions)
    ↓
    ├─→ swarm_nav_slam (SLAM & Graph Merging)
    ├─→ swarm_nav_coordination (Frontier Detection & Auction)
    ├─→ swarm_nav_navigation (ORCA Filter & Obstacle Tracking)
    └─→ swarm_nav_evaluation (Metrics Collection)
    
swarm_nav_bringup (Launch Files & Configuration)
    ├─ Depends on: All above packages
    └─ Integrates: Nav2, Gazebo, RViz
```

---

## Algorithm Flow

### 1. Exploration Loop (Per Robot)

```
┌─────────────────────────────────────────────────────────────┐
│ 1. SLAM updates map from sensor data                        │
│    ↓                                                         │
│ 2. Frontier detector finds unexplored boundaries            │
│    ↓                                                         │
│ 3. Calculate utility for each frontier                      │
│    utility = size × distance_weight                          │
│    ↓                                                         │
│ 4. Auctioneer announces auction for best frontier           │
│    ↓                                                         │
│ 5. All robots calculate bid cost                            │
│    bid_cost = distance / nominal_speed                       │
│    ↓                                                         │
│ 6. Winner determined (lowest bid, tie-break by robot_id)    │
│    ↓                                                         │
│ 7. Winner navigates to frontier                             │
│    ↓                                                         │
│ 8. ORCA filter ensures collision-free motion                │
│    ↓                                                         │
│ 9. Repeat until map fully explored                          │
└─────────────────────────────────────────────────────────────┘
```

### 2. ORCA Collision Avoidance

```
For each robot:
  1. Get current velocity and position
  2. Get neighbor states from /swarm/neighbor_states
  3. For each neighbor:
     - Calculate relative position and velocity
     - Compute velocity obstacle (VO)
     - Find collision-free velocity
  4. Select optimal velocity closest to desired
  5. Publish safe velocity to /robot_X/cmd_vel
```

### 3. Auction Protocol

```
Auctioneer (Robot with frontier):
  1. Detect frontier
  2. Announce auction: /swarm/auction/announce
  3. Wait for bids (timeout: 500ms)
  4. Select winner (lowest bid)
  5. Publish result: /swarm/auction/result

Bidder (All robots):
  1. Receive auction announcement
  2. Calculate bid cost = distance / speed
  3. Publish bid: /swarm/auction/bid
  4. Wait for result
  5. If winner: navigate to frontier
```

---

## Key Design Decisions

1. **Centralized State Aggregation**: Single aggregator node collects and distributes neighbor states to avoid N² topic subscriptions

2. **Simplified Map Merging**: Occupancy grid overlay on rendezvous instead of complex pose graph alignment

3. **Deterministic Tie-Breaking**: Robot ID used as tie-breaker in auctions for reproducible behavior

4. **Classification-Aware Obstacles**: Different decay rates for STATIC, SEMI_DYNAMIC, and DYNAMIC obstacles

5. **BehaviorTree Integration**: Nav2 BT nodes orchestrate exploration behavior with live topic wiring

---

## Performance Characteristics

- **Scalability**: 3-5 robots (tested)
- **Map Size**: 40m × 60m warehouse
- **Coverage Target**: ≥95% in ≤10 minutes
- **SLAM Accuracy**: ATE <0.3m RMSE
- **Collision Rate**: 0 collisions (target)
- **Communication**: ~10 Hz state updates
- **Computation**: Real-time on standard hardware

---

## File Structure

```
SwarmNav-Sim/
├── src/
│   ├── swarm_nav_msgs/          # Custom message definitions
│   ├── swarm_nav_slam/          # SLAM and graph merging
│   ├── swarm_nav_coordination/  # Frontier detection & auction
│   ├── swarm_nav_navigation/    # ORCA filter & obstacle tracking
│   ├── swarm_nav_evaluation/    # Metrics collection
│   └── swarm_nav_bringup/       # Launch files & config
│       ├── launch/
│       │   ├── swarm.launch.py       # Main launch file
│       │   ├── gazebo.launch.py      # Simulator launch
│       │   └── evaluation.launch.py  # Metrics launch
│       ├── config/
│       │   ├── robot_nav2.yaml       # Nav2 parameters
│       │   └── mrg_slam_multirobot.yaml  # SLAM config
│       ├── worlds/
│       │   └── warehouse.world       # Gazebo world
│       └── urdf/
│           └── swarm_robot.urdf.xacro  # Robot description
├── QUICKSTART.md                # 5-minute getting started
├── RUNNING_INSTRUCTIONS.md      # Comprehensive guide
├── IMPLEMENTATION_SUMMARY.md    # Technical details
└── README.md                    # Project overview
```

---

**For detailed implementation notes, see [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)**

**For running instructions, see [RUNNING_INSTRUCTIONS.md](RUNNING_INSTRUCTIONS.md)**

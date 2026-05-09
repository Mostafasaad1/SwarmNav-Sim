# Quickstart: Complete Core Nodes

## Prerequisites

- Workspace from `001-swarm-nav-sim` must be built and sourced
- All `swarm_nav_msgs` must be generated (run `colcon build --packages-select swarm_nav_msgs` first)

## Build Verification

```bash
# Full workspace build
cd /home/mox/projects/SwarmNav-Sim
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash

# Quick check — verify no missing symbols
ros2 run swarm_nav_coordination frontier_detector_node --ros-args -p robot_id:=test &
sleep 2
kill %1
echo "frontier_detector_node: OK"
```

## Smoke Test: Deadlock Verification (US1)

```bash
# Terminal 1: Launch ORCA filter
ros2 run swarm_nav_navigation orca_velocity_filter_node --ros-args -p robot_id:=robot_0

# Terminal 2: Inject simultaneous data (should not hang)
ros2 topic pub --once /swarm/neighbor_states swarm_nav_msgs/msg/NeighborStateArray "{header: {}, neighbors: [{robot_id: 'robot_1', pose: {position: {x: 1.0}}, velocity: {}, radius: 0.25}]}"
ros2 topic pub --once /robot_0/cmd_vel_nav2 geometry_msgs/msg/Twist "{linear: {x: 0.5}}"

# Verify: node should output debug logs, not hang
```

## Smoke Test: Obstacle Pipeline (US2)

```bash
# Terminal 1: Launch tracker
ros2 run swarm_nav_navigation obstacle_tracker_node

# Terminal 2: Verify messages
ros2 topic echo /swarm/tracked_obstacles --once
# Expect: ObstacleArray with 8 obstacles (3 forklifts + 5 humans)
```

## Smoke Test: Auction Determinism (US3)

```bash
# Terminal 1: Launch auctioneer
ros2 run swarm_nav_coordination auctioneer_node --ros-args -p robot_id:=robot_0

# Terminal 2: Inject frontiers (should trigger auction)
ros2 topic pub --once /robot_0/frontiers swarm_nav_msgs/msg/FrontierArray "{header: {}, frontiers: [{centroid: {x: 5.0, y: 3.0}, size: 50, utility: 30, frontier_id: 'f1'}]}"

# Verify: Auction announce appears on /swarm/auction/announce
ros2 topic echo /swarm/auction/announce --once
```

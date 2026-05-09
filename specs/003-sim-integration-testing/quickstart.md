# Quickstart: Simulator Integration & Full System Testing

**Feature**: `003-sim-integration-testing`

---

## Prerequisites

- Ubuntu 22.04
- ROS 2 Humble (base install from `ros-humble-desktop`)
- `colcon` build tools

## 1. Install Dependencies

```bash
cd /path/to/SwarmNav-Sim
./setup_dependencies.sh
```

This installs BehaviorTree.CPP v4, Nav2, Gazebo Fortress, and all rosdep dependencies.

To see what would be installed without executing:
```bash
./setup_dependencies.sh --dry-run
```

## 2. Build the Workspace

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```

**Expected**: All packages build, including BT nodes and DynamicObstacleLayer plugin.

## 3. Launch the Simulation

### Gazebo Fortress (default)

```bash
ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3 simulator:=gazebo
```

### CoppeliaSim (optional, requires external install)

```bash
ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3 simulator:=coppeliasim
```

## 4. Verify System is Running

In a new terminal:
```bash
source install/setup.bash

# Check all robot topics exist
ros2 topic list | grep robot_0
# Expected: /robot_0/scan, /robot_0/odom, /robot_0/cmd_vel, ...

# Check obstacle tracker
ros2 hz /swarm/tracked_obstacles
# Expected: ~10 Hz

# Check auction results
ros2 topic echo /swarm/auction/result --once
# Expected: AuctionResult with winner_id and frontier_id
```

## 5. Run the Evaluation

```bash
# In a new terminal while simulation is running:
ros2 run swarm_nav_evaluation coverage_evaluator
ros2 run swarm_nav_evaluation collision_monitor
```

After 5 minutes, check the output files:
- `coverage_results.json` — coverage percentage over time
- `collision_results.json` — collision events (should be empty)

## 6. Run Automated Tests

```bash
# Unit tests (no simulator needed)
colcon test --packages-select swarm_nav_msgs swarm_nav_coordination swarm_nav_navigation swarm_nav_slam
colcon test-result --verbose

# Integration tests (requires Gazebo)
colcon test --packages-select swarm_nav_bringup
colcon test-result --verbose
```

## Smoke Tests

| Test | Command | Expected |
|------|---------|----------|
| Build succeeds | `colcon build` | Exit 0, no errors |
| BT nodes built | `ls install/swarm_nav_coordination/lib/libswarm_bt_nodes.so` | File exists |
| Costmap plugin built | `ls install/swarm_nav_navigation/lib/libdynamic_obstacle_layer.so` | File exists |
| Sim launches | `ros2 launch swarm_nav_bringup swarm.launch.py` | No crash for 30s |
| Topics publish | `ros2 topic hz /robot_0/scan` | >0 Hz |
| Coverage test | `colcon test --packages-select swarm_nav_bringup` | All tests pass |

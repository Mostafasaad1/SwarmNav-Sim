# SwarmNav-Sim Running Instructions

Complete guide to building, testing, and running the multi-robot swarm navigation system.

---

## Prerequisites

### System Requirements
- **OS**: Ubuntu 22.04 LTS
- **ROS 2**: Humble Hawksbill
- **Python**: 3.10+
- **Build Tools**: colcon, CMake 3.16+

### Required Dependencies

The project includes an automated dependency installation script:

```bash
# Install all dependencies (requires sudo)
./setup_dependencies.sh

# Or install without simulator (faster, for development/testing only)
./setup_dependencies.sh --no-sim

# Dry run to see what would be installed
./setup_dependencies.sh --dry-run
```

**What gets installed:**
- ROS 2 Humble packages (nav2, slam_toolbox, etc.)
- BehaviorTree.CPP v4
- Ignition Gazebo (Fortress) (unless `--no-sim` is used)
- Python dependencies (numpy, scipy, etc.)

---

## Building the Workspace

### 1. Source ROS 2 Environment

```bash
source /opt/ros/humble/setup.zsh  # or setup.bash for bash users
```

### 2. Build All Packages

```bash
# Build everything
colcon build

# Or build specific packages
colcon build --packages-select swarm_nav_msgs swarm_nav_slam swarm_nav_navigation

# Build with verbose output for debugging
colcon build --event-handlers console_direct+
```

### 3. Source the Workspace

```bash
# For zsh users
source install/setup.zsh

# For bash users
source install/setup.bash
```

**Note**: You must source the workspace in every new terminal before running any ROS 2 commands.

---

## Running the System

### Option 1: Ignition Gazebo Simulator

Launch Ignition Gazebo with the warehouse world:

```bash
# Source the workspace first
source install/setup.zsh

# Launch Ignition Gazebo with GUI (default)
ros2 launch swarm_nav_bringup ignition.launch.py

# Launch without GUI (headless)
ros2 launch swarm_nav_bringup ignition.launch.py gui:=false

# Launch with verbose output
ros2 launch swarm_nav_bringup ignition.launch.py verbose:=true
```

**What this launches:**
- Ignition Gazebo (Fortress) with warehouse world
- Physics simulation with walls and obstacles
- 3D visualization (if gui:=true)
- Ready for robot spawning

### Option 2: Development Mode (No Simulator)

For testing without 3D simulation:

```bash
# Launch without simulator
ros2 launch swarm_nav_bringup swarm.launch.py \
  simulator:=none \
  num_robots:=3
```

**Note**: You'll need to publish mock sensor data manually for this mode.

---

## Running Evaluation Nodes

To collect metrics during exploration missions:

```bash
# In a new terminal (after sourcing the workspace)
ros2 launch swarm_nav_evaluation evaluation.launch.py \
  num_robots:=3 \
  duration:=300 \
  output_dir:=/tmp/swarm_results
```

**Evaluation metrics collected:**
- **Coverage**: Map exploration percentage over time
- **Collisions**: Robot-robot and robot-obstacle collision events
- **SLAM Accuracy**: Absolute Trajectory Error (ATE) if ground truth available

**Output files** (saved to `output_dir`):
- `coverage_results.json` - Coverage percentage history
- `collision_results.json` - Collision events log
- `slam_metrics.json` - SLAM performance metrics

---

## Testing

### Run All Tests

```bash
# Build first
colcon build

# Run all tests
colcon test

# View test results
colcon test-result --verbose
```

### Run Tests for Specific Package

```bash
# Test a single package
colcon test --packages-select swarm_nav_slam

# View results
colcon test-result --verbose --packages-select swarm_nav_slam
```

### Run Integration Tests

**Note**: Integration tests require Ignition Gazebo to be installed and running.

```bash
# Run integration tests
colcon test --packages-select swarm_nav_bringup

# Run with output on failure
colcon test --packages-select swarm_nav_bringup --event-handlers console_direct+
```

### Test Categories

1. **Unit Tests** (C++ with gtest):
   - `test_auctioneer` - Bid calculation logic
   - `test_orca_filter` - Collision avoidance velocity filtering
   - `test_obstacle_tracker` - Dynamic obstacle tracking
   - `test_graph_merge` - Multi-robot SLAM graph merging

2. **Unit Tests** (Python with pytest):
   - `test_collision_monitor` - Collision detection logic
   - `test_coverage_evaluator` - Coverage calculation

3. **Integration Tests** (launch_test):
   - `test_system_launch` - Full system startup and topic publishing
   - `test_topic_publishing` - Robot motion via cmd_vel

4. **Linting Tests**:
   - `flake8` - Python code style
   - `pep257` - Python docstring style
   - `uncrustify` - C++ code formatting
   - `cpplint` - C++ code style
   - `lint_cmake` - CMake formatting

---

## Monitoring and Debugging

### View Active Topics

```bash
# List all topics
ros2 topic list

# Monitor robot odometry
ros2 topic echo /robot_0/odom

# Monitor frontier detections
ros2 topic echo /robot_0/frontiers

# Monitor neighbor states
ros2 topic echo /swarm/neighbor_states
```

### View Active Nodes

```bash
# List all nodes
ros2 node list

# Get info about a specific node
ros2 node info /robot_0/auctioneer_node
```

### View TF Tree

```bash
# Install if needed
sudo apt install ros-humble-tf2-tools

# View TF tree
ros2 run tf2_tools view_frames

# Monitor specific transform
ros2 run tf2_ros tf2_echo map robot_0/base_link
```

### Check Node Logs

```bash
# View logs for a specific node
ros2 run rqt_console rqt_console

# Or use command line
ros2 topic echo /rosout
```

### Visualize in RViz

If you didn't launch with `use_rviz:=true`, you can start RViz manually:

```bash
ros2 run rviz2 rviz2 -d src/swarm_nav_bringup/rviz/swarm_nav.rviz
```

**RViz displays:**
- Robot models and TF frames
- Lidar scans
- Occupancy grid maps
- Planned paths
- Frontier markers
- Neighbor robot positions

---

## Common Issues and Solutions

### Issue: "No module named 'swarm_nav_msgs'"

**Solution**: Build and source the workspace:
```bash
colcon build --packages-select swarm_nav_msgs
source install/setup.zsh
```

### Issue: "package 'ros_gz_sim' not found"

**Solution**: Install Ignition Gazebo dependencies:
```bash
./setup_dependencies.sh
```

### Issue: Gazebo crashes or doesn't start

**Solution**: 
1. Check if Gazebo is already running: `ps aux | grep gz`
2. Kill existing instances: `killall gz`
3. Try launching again

### Issue: Robots don't move

**Possible causes:**
1. Navigation stack not initialized - check logs for lifecycle manager errors
2. No frontiers detected - map might be fully explored
3. SLAM not working - check if `/robot_X/map` topics are publishing

**Debug steps:**
```bash
# Check if cmd_vel is being published
ros2 topic hz /robot_0/cmd_vel

# Check if map is being published
ros2 topic hz /robot_0/map

# Check navigation status
ros2 topic echo /robot_0/behavior_tree_log
```

### Issue: High CPU usage

**Solutions:**
1. Run headless mode: `ros2 launch swarm_nav_bringup ignition.launch.py gui:=false`
2. Reduce Ignition Gazebo rendering quality in GUI settings
3. Close other applications

---

## Performance Tuning

### Adjust Navigation Parameters

Edit `src/swarm_nav_bringup/config/robot_nav2.yaml`:

```yaml
controller_server:
  ros__parameters:
    FollowPath:
      max_vel_x: 0.5  # Increase for faster movement
      max_vel_theta: 1.0
```

### Adjust ORCA Parameters

Edit parameters in `swarm.launch.py`:

```python
parameters=[{
    'robot_radius': 0.25,  # Increase for more conservative avoidance
    'time_horizon': 2.0,   # Increase for smoother avoidance
    'max_neighbors': 10
}]
```

### Adjust Frontier Detection

Edit parameters in `swarm.launch.py`:

```python
parameters=[{
    'min_frontier_size': 10,  # Decrease to detect smaller frontiers
    'frontier_travel_point_distance': 1.0
}]
```

---

## Stopping the System

### Graceful Shutdown

Press `Ctrl+C` in the terminal where you launched the system. This will:
1. Stop all ROS 2 nodes
2. Close Gazebo
3. Clean up resources

### Force Kill (if needed)

```bash
# Kill all ROS 2 nodes
killall -9 ros2

# Kill Gazebo
killall -9 gz

# Kill RViz
killall -9 rviz2
```

---

## Next Steps

1. **Experiment with parameters**: Modify launch file parameters to see how they affect behavior
2. **Add custom worlds**: Create new `.world` files in `src/swarm_nav_bringup/worlds/`
3. **Implement new behaviors**: Add custom behavior trees in Nav2 configuration
4. **Collect data**: Run evaluation nodes and analyze the JSON output files
5. **Scale up**: Test with more robots (up to 5 supported)

---

## Additional Resources

- **ROS 2 Documentation**: https://docs.ros.org/en/humble/
- **Nav2 Documentation**: https://navigation.ros.org/
- **Gazebo Documentation**: https://gazebosim.org/docs
- **Project README**: See `README.md` for architecture overview
- **Implementation Details**: See `IMPLEMENTATION_SUMMARY.md`

---

## Quick Reference Commands

```bash
# Build workspace
colcon build

# Source workspace (zsh)
source install/setup.zsh

# Launch full system
ros2 launch swarm_nav_bringup swarm.launch.py simulator:=gazebo num_robots:=3

# Run tests
colcon test && colcon test-result --verbose

# List topics
ros2 topic list

# Monitor a topic
ros2 topic echo /robot_0/odom

# View node graph
rqt_graph
```

---

**Last Updated**: May 9, 2026

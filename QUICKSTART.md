# SwarmNav-Sim Quick Start Guide

Get up and running with the multi-robot swarm navigation system in 5 minutes.

---

## Prerequisites Check

```bash
# Check ROS 2 installation
ros2 --version
# Expected: ros2 doctor version 0.10.4 (or similar)

# Check if workspace exists
ls src/swarm_nav_*/
# Should list: swarm_nav_bringup, swarm_nav_coordination, etc.
```

---

## Step 1: Install Dependencies (First Time Only)

```bash
# Install all dependencies including Gazebo (requires sudo)
./setup_dependencies.sh

# This installs:
# - Nav2 navigation stack
# - BehaviorTree.CPP v4
# - Ignition Gazebo (Fortress) simulator
# - Python packages (numpy, scipy)
```

**Estimated time**: 5-10 minutes

---

## Step 2: Build the Workspace

```bash
# Source ROS 2
source /opt/ros/humble/setup.zsh  # or setup.bash

# Build all packages
colcon build

# Source the workspace
source install/setup.zsh  # or setup.bash
```

**Estimated time**: 2-3 minutes

---

## Step 3: Launch the System

```bash
# Launch Ignition Gazebo with 3 robots
ros2 launch swarm_nav_bringup ignition_with_robots.launch.py num_robots:=3
```

**What you'll see:**
1. Ignition Gazebo window opens with warehouse environment
2. 3 robots spawn at different positions in the warehouse
3. Robots are visible but won't move yet (navigation nodes disabled)
4. Terminal shows robot spawning logs

**Estimated time**: 30 seconds to start

---

## Step 4: Watch the Robots Explore

The robots will automatically:
1. **Build maps** using SLAM (Simultaneous Localization and Mapping)
2. **Detect frontiers** (unexplored areas)
3. **Auction tasks** to decide which robot explores which frontier
4. **Navigate** to assigned frontiers while avoiding collisions
5. **Share information** about explored areas

### What to Look For:

**In Gazebo:**
- Robots moving around the warehouse
- Lidar beams scanning the environment

**In RViz:**
- Maps filling in as robots explore (gray = unknown, white = free, black = obstacle)
- Green markers showing detected frontiers
- Colored paths showing planned routes
- Robot models with TF frames

**In Terminal:**
- Log messages about frontier detection
- Auction bid announcements
- Navigation status updates

---

## Step 5: Monitor Progress (Optional)

Open a new terminal and run:

```bash
# Source the workspace first
source install/setup.zsh

# Watch robot positions
ros2 topic echo /robot_0/odom --once

# Watch frontier detections
ros2 topic echo /robot_0/frontiers --once

# Watch auction bids
ros2 topic echo /swarm/bids --once

# List all active topics
ros2 topic list
```

---

## Step 6: Run Evaluation (Optional)

To collect performance metrics:

```bash
# In a new terminal (source workspace first)
source install/setup.zsh

# Launch evaluation nodes
ros2 launch swarm_nav_evaluation evaluation.launch.py \
  num_robots:=3 \
  duration:=300

# Results saved to:
# - coverage_results.json (exploration coverage over time)
# - collision_results.json (collision events)
# - slam_metrics.json (SLAM accuracy)
```

---

## Step 7: Stop the System

Press `Ctrl+C` in the launch terminal to gracefully shut down all nodes.

---

## Troubleshooting

### Problem: "ign command not found"

**Solution**: Ignition Gazebo not installed. Run:
```bash
./setup_dependencies.sh
```

### Problem: Robots don't move

**Check 1**: Are maps being published?
```bash
ros2 topic hz /robot_0/map
```

**Check 2**: Are frontiers detected?
```bash
ros2 topic echo /robot_0/frontiers
```

**Check 3**: Is navigation active?
```bash
ros2 node list | grep controller
```

### Problem: Ignition Gazebo crashes

**Solution**: Kill existing instances and restart:
```bash
killall ign
ros2 launch swarm_nav_bringup ignition.launch.py
```

### Problem: High CPU usage

**Solution**: Run in headless mode:
```bash
ros2 launch swarm_nav_bringup ignition.launch.py gui:=false
```

---

## Next Steps

### Try Different Configurations

```bash
# Launch with GUI (default)
ros2 launch swarm_nav_bringup ignition.launch.py

# Headless mode (no GUI, faster)
ros2 launch swarm_nav_bringup ignition.launch.py gui:=false

# Verbose output for debugging
ros2 launch swarm_nav_bringup ignition.launch.py verbose:=true
```

### Run Tests

```bash
# Run all tests
colcon test

# View results
colcon test-result --verbose

# Run specific package tests
colcon test --packages-select swarm_nav_slam
```

### Modify Parameters

Edit configuration files:
- **Navigation**: `src/swarm_nav_bringup/config/robot_nav2.yaml`
- **SLAM**: `src/swarm_nav_bringup/config/mrg_slam_multirobot.yaml`
- **Launch parameters**: `src/swarm_nav_bringup/launch/swarm.launch.py`

After editing, rebuild:
```bash
colcon build --packages-select swarm_nav_bringup
source install/setup.zsh
```

---

## Command Cheat Sheet

```bash
# Build
colcon build

# Source (do this in every new terminal)
source install/setup.zsh

# Launch
ros2 launch swarm_nav_bringup ignition.launch.py

# Test
colcon test && colcon test-result --verbose

# Monitor topics
ros2 topic list
ros2 topic echo /robot_0/odom
ros2 topic hz /robot_0/scan

# Monitor nodes
ros2 node list
ros2 node info /robot_0/auctioneer_node

# Stop everything
Ctrl+C (in launch terminal)
```

---

## Getting Help

- **Full documentation**: See `RUNNING_INSTRUCTIONS.md`
- **Architecture overview**: See `README.md`
- **Implementation details**: See `IMPLEMENTATION_SUMMARY.md`
- **ROS 2 docs**: https://docs.ros.org/en/humble/
- **Nav2 docs**: https://navigation.ros.org/

---

**Enjoy exploring with your robot swarm! 🤖🤖🤖**

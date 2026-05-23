# Testing & Verification Guide: Mission Executor System

This guide outlines how to fully test and verify the autonomous, decentralized exploration behaviors introduced by the `mission_executor` node in your SwarmNav-Sim environment.

## 1. Initial Setup and Compilation

Ensure your workspace is fully built and sourced:
```bash
cd ~/projects/SwarmNav-Sim
colcon build --symlink-install
source install/setup.bash
```

## 2. Launching the Full Swarm Scenario

The easiest way to test the system is to launch the entire swarm. The launch file automatically instantiates the `mission_executor_node` for each robot and hands control over to the Nav2 `lifecycle_manager`.

**Launch Command:**
```bash
ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3 use_rviz:=true gui:=true
```

> [!TIP]
> If your machine struggles with performance, drop the robot count to 1 or 2 by changing `num_robots:=1` and disable the Gazebo GUI with `gui:=false`.

## 3. What to Watch For During Startup

1. **Nav2 Lifecycle Bringup**:
   Watch the terminal output for lines like:
   `[lifecycle_manager_navigation]: Starting managed nodes bringup...`
   You should see `controller_server`, `planner_server`, `behavior_server`, `bt_navigator`, and finally `mission_executor` transitioning to `Active`.

2. **Mission Executor Activation**:
   Look for the confirmation that the Behavior Tree engine has successfully booted:
   ```text
   [robot_0.mission_executor]: Activating mission executor
   [robot_0.mission_executor]: Activation complete – ticking at 10.0 Hz
   ```

## 4. Validating the Exploration Logic (Behavior Tree)

Once activated, the robots will begin executing the `mission_tree.xml` loop:

1. **Frontier Detection:**
   The BT triggers `FrontierDetectorBT`. If the map is still mostly unexplored, you'll see:
   `[robot_0.mission_executor]: Detected X frontiers`
2. **Auction Mechanism:**
   The BT triggers `RunAuctionBT`. The robot listens to `/swarm/auction/result`. When it wins a bid, it will log:
   `[robot_0.mission_executor]: Won auction – navigating to frontier UUID (x.xx, y.yy)`
3. **Navigation Execution:**
   The BT then executes `NavigateToPose`, handing the goal to Nav2. You'll see Nav2's path planner generate a route, and the robot will physically move in Gazebo and RViz.

## 5. Visualizing the System in RViz2

If you passed `use_rviz:=true` to the launch command, RViz will open.

- **Check Global Costmap**: Ensure the global map is updating as robots move into unknown areas.
- **Check Frontier Markers**: You should see colored markers (usually spheres or cubes depending on your configuration) appearing at the edges of the known map. These represent the frontiers that robots are bidding on.
- **Check Navigation Goals**: You should see the standard Nav2 green arrows representing the active goals (frontiers) the robots are moving toward.

## 6. Manual Introspection & Debugging

If the robots aren't moving or behave unexpectedly, use these commands in a new terminal:

### Check Lifecycle States
Ensure the mission executor is `active`:
```bash
ros2 lifecycle get /robot_0/mission_executor
```

### Trace the Auction Data
Verify that frontiers are being published and auction results are being processed:
```bash
# Check if frontiers are actively detected
ros2 topic echo /robot_0/frontiers --noarr

# Check who is winning the auctions
ros2 topic echo /swarm/auction/result
```

### Verify Goal Passing
If the auction is won but the robot doesn't move, ensure `NavigateToPose` is receiving goals:
```bash
ros2 action list
ros2 topic echo /robot_0/navigate_to_pose/_action/goal
```

> [!WARNING]
> If you see `[tf2_buffer]: Detected jump back in time` warnings constantly spamming the log and interrupting navigation, this is an issue with Gazebo/ROS2 simulation time synchronization. Try restarting the ROS 2 daemon (`ros2 daemon stop`) and relaunching.

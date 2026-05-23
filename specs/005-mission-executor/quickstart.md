# Quickstart: Mission Executor

To run the full autonomous decentralized exploration scenario:

1. Build the workspace:
   ```bash
   colcon build --symlink-install
   ```

2. Source the workspace:
   ```bash
   source install/setup.zsh
   ```

3. Launch the swarm (which now automatically includes the `mission_executor` for each robot):
   ```bash
   ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3
   ```

4. The nodes will boot, Nav2 will initialize, and the lifecycle manager will transition the `mission_executor` nodes to `Active`. The robots will immediately begin exploring the map autonomously using the frontiers and auction mechanism.

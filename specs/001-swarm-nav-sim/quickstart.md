# SwarmNav-Sim Quickstart

## Prerequisites
- Ubuntu 22.04 LTS
- ROS 2 Humble Hawksbill (desktop install)
- NVIDIA Isaac Sim 5.0 (or Gazebo Harmonic)
- `colcon` build tool

## Building the Workspace

1. Clone the repository and its submodules (e.g., `mrg_slam`, `nav2`, `RVO2-ROS2`).
2. Install rosdep dependencies:
   ```bash
   rosdep install --from-paths src --ignore-src -r -y
   ```
3. Build the workspace:
   ```bash
   colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
   ```
4. Source the install environment:
   ```bash
   source install/setup.bash
   ```

## Running the Simulation

1. **Start the ROS 2 Discovery Server**:
   ```bash
   fastdds discovery -i 0 -l 127.0.0.1 -p 11811
   ```

2. **Launch the Swarm Environment**:
   In a new terminal, export the discovery server and launch the simulation:
   ```bash
   export ROS_DISCOVERY_SERVER="127.0.0.1:11811"
   source install/setup.bash
   ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=5
   ```
   *This launch file starts Isaac Sim, spawns 5 robots, launches 3 forklift NPCs, 5 human NPCs, and brings up the Nav2, mrg_slam, and behavior tree nodes for each robot.*

3. **Monitor via RViz**:
   The launch file automatically opens RViz configured to display `/swarm/global_map`, individual robot footprints, frontiers, and TEB local plans.

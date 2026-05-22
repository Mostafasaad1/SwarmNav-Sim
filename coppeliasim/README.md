# CoppeliaSim Backend for SwarmNav-Sim

This directory contains assets for running SwarmNav-Sim with CoppeliaSim as an alternative to Gazebo.

## Installation

1. Download and install [CoppeliaSim Edu](https://www.coppeliarobotics.com/).
2. Set the `COPPELIASIM_ROOT_DIR` environment variable to your installation path:
   ```bash
   export COPPELIASIM_ROOT_DIR=/path/to/coppeliasim
   ```
3. Install the ROS 2 plugin `simROS2`:
   - Follow instructions at https://github.com/CoppeliaRobotics/simROS2 to compile the plugin for ROS 2 Humble/Jazzy.
   - The compiled `libsimExtROS2.so` must be placed in your CoppeliaSim installation directory.

## Usage

To use CoppeliaSim as the simulator backend, simply pass `simulator:=coppeliasim` to the main launch file:

```bash
ros2 launch swarm_nav_bringup swarm.launch.py simulator:=coppeliasim num_robots:=3
```

Note: You must manually launch CoppeliaSim and load the `scenes/warehouse.ttt` scene before running the ROS 2 launch file.

## Features

- Fully compatible ROS 2 topic interface.
- Proxies `cmd_vel`, `odom`, and `scan` natively via Lua scripts in the scene.

# SwarmNav-Sim: Decentralized Multi-Robot Warehouse Exploration

A ROS 2 Humble simulation system for decentralized multi-robot warehouse exploration using classical navigation and coordination algorithms.

## Features

- **Multi-Robot Collaborative SLAM**: Robots independently map surroundings and merge pose graphs peer-to-peer using mrg_slam
- **Decentralized Task Allocation**: Vickrey auction-based frontier assignment orchestrated by BehaviorTree.CPP v4
- **Classical Dynamic Obstacle Avoidance**: Nav2 TEB planner with ORCA (RVO2) velocity filtering - no machine learning

## System Architecture

- **3-5 robots** exploring a 40m x 60m warehouse
- **Dynamic obstacles**: 3 forklifts, 5 human NPCs
- **Performance targets**: ≥95% coverage in ≤10 minutes, SLAM ATE <0.3m RMSE, 0 collisions

## Prerequisites

- Ubuntu 22.04 LTS
- ROS 2 Humble Hawksbill (desktop install)
- NVIDIA Isaac Sim 5.0 (or Gazebo Harmonic)
- colcon build tool

## Quick Start

See [quickstart.md](specs/001-swarm-nav-sim/quickstart.md) for detailed build and run instructions.

### Build

```bash
# Install dependencies
rosdep install --from-paths src --ignore-src -r -y

# Build workspace
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release

# Source the workspace
source install/setup.bash
```

### Run

```bash
# Terminal 1: Start ROS 2 Discovery Server
./scripts/start_discovery_server.sh

# Terminal 2: Launch simulation with 5 robots
export ROS_DISCOVERY_SERVER="127.0.0.1:11811"
source install/setup.bash
ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=5

# Terminal 3: Launch evaluation nodes
export ROS_DISCOVERY_SERVER="127.0.0.1:11811"
source install/setup.bash
ros2 launch swarm_nav_evaluation evaluation.launch.py num_robots:=5
```

### Monitor Results

Evaluation results are saved to:
- `coverage_results.json` - Map coverage over time
- `collision_results.json` - Collision events
- `slam_metrics.json` - SLAM accuracy (ATE RMSE)

## Configuration and Tuning

See [TUNING.md](TUNING.md) for detailed parameter tuning guidance.

## Package Structure

- `swarm_nav_bringup`: Launch files, RViz configs, parameters
- `swarm_nav_slam`: SLAM configuration and graph merging
- `swarm_nav_navigation`: Custom TEB layer, ORCA filter node
- `swarm_nav_coordination`: BehaviorTree XML, auction nodes, frontier detector
- `swarm_nav_msgs`: Custom message definitions
- `swarm_nav_evaluation`: Coverage, collision, and SLAM metrics

## Documentation

- [Specification](specs/001-swarm-nav-sim/spec.md)
- [Implementation Plan](specs/001-swarm-nav-sim/plan.md)
- [Data Model](specs/001-swarm-nav-sim/data-model.md)
- [DDS Topics](specs/001-swarm-nav-sim/contracts/dds-topics.md)
- [Research Notes](specs/001-swarm-nav-sim/research.md)

## License

TODO: Add license

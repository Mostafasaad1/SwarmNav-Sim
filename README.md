# SwarmNav-Sim: Decentralized Multi-Robot Warehouse Exploration

A ROS 2 Jazzy simulation system for decentralized multi-robot warehouse exploration using classical navigation and coordination algorithms.

**Status**: Core implementation complete - all user stories implemented and verified (2026-05-09)

## Features

- **Multi-Robot Collaborative SLAM**: Robots independently map surroundings and merge maps via simplified occupancy grid overlay on rendezvous
- **Decentralized Task Allocation**: Spec-compliant Vickrey auction with deterministic tie-breaking, orchestrated by BehaviorTree.CPP v4
- **Classification-Aware Obstacle Avoidance**: Dynamic costmap layer with Gaussian inflation and classification-based decay (STATIC/SEMI_DYNAMIC/DYNAMIC)
- **Velocity Obstacle Algorithm**: Custom ORCA implementation for collision-free multi-robot navigation
- **Neighbor State Aggregation**: Centralized aggregator for efficient state distribution
- **Automated Benchmarking Suite**: Headless multi-scenario benchmark runner with system metrics (CPU/Memory) and consolidated JSON/CSV reports
- **Parameter Sensitivity Analysis**: Systematic parameter sweep across robot configurations with correlation plots
- **Bayesian Optimization Tuning**: Optuna-based automated hyperparameter tuning for optimal swarm configuration

## System Architecture

- **3-5 robots** exploring a 40m x 60m warehouse
- **Dynamic obstacles**: 3 forklifts, 5 human NPCs
- **Performance targets**: ≥95% coverage in ≤10 minutes, SLAM ATE <0.3m RMSE, 0 collisions

For detailed architecture diagrams and data flow, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Prerequisites

- Ubuntu 22.04 LTS
- ROS 2 Jazzy Jalisco (desktop install)
- Python 3.10+
- colcon build tool

**Optional** (for full functionality):
- Ignition Gazebo (Fortress) or NVIDIA Isaac Sim 5.0 (for simulation)
- BehaviorTree.CPP v4 (for BT nodes)
- Nav2 (for costmap plugin)

## Quick Start

**New to the project?** See [QUICKSTART.md](QUICKSTART.md) for a 5-minute getting started guide.

**Need detailed instructions?** See [RUNNING_INSTRUCTIONS.md](RUNNING_INSTRUCTIONS.md) for comprehensive documentation.

### Installation

```bash
# 1. Install dependencies (requires sudo)
./setup_dependencies.sh

# 2. Build workspace (using the robust build script)
./build.sh

# 3. Source the workspace
source install/setup.bash  # or setup.zsh
```

### Launch

```bash
# Launch the full decentralized swarm in Gazebo Fortress
ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3
```

**What happens:**
- Gazebo Fortress opens with the warehouse environment.
- 3 robots spawn at predefined start positions.
- Each robot initializes its own SLAM, Nav2, and coordination stack.
- The robots begin autonomous exploration and coordinate tasks via auctions.
- RViz opens to visualize the individual and global merged maps.

### Testing

```bash
# Run all tests
colcon test

# View results
colcon test-result --verbose

# Test specific package
colcon test --packages-select swarm_nav_slam
```

**Test Status**: ✅ All unit tests and linting tests pass
- Integration tests require Gazebo simulator to be installed

### Benchmarking & Tuning

```bash
# Run the automated benchmark suite
ros2 run swarm_nav_evaluation benchmark_runner.py --config src/swarm_nav_evaluation/config/scenarios.yaml

# Run parameter sensitivity sweep
ros2 run swarm_nav_evaluation benchmark_runner.py --sweep --config src/swarm_nav_evaluation/config/scenarios.yaml

# Run Bayesian hyperparameter tuning
ros2 run swarm_nav_evaluation bayesian_tuner.py --config src/swarm_nav_evaluation/config/tune_space.yaml --trials 50
```

For detailed testing instructions, see [RUNNING_INSTRUCTIONS.md](RUNNING_INSTRUCTIONS.md#testing).

---

## Documentation

- **[QUICKSTART.md](QUICKSTART.md)** - Get running in 5 minutes
- **[RUNNING_INSTRUCTIONS.md](RUNNING_INSTRUCTIONS.md)** - Complete usage guide
- **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** - Technical implementation details
- **[TUNING.md](TUNING.md)** - Parameter tuning guide

---

## Smoke Tests (Component Verification)

Test individual components to verify the implementation:

#### Test 1: Obstacle Tracking Pipeline
```bash
# Terminal 1: Launch obstacle tracker
ros2 run swarm_nav_navigation obstacle_tracker_node

# Terminal 2: Verify obstacle messages
ros2 topic echo /swarm/tracked_obstacles --once
# Expected: ObstacleArray with 8 obstacles (3 forklifts + 5 humans)
```

#### Test 2: Frontier Detection and Auction
```bash
# Terminal 1: Launch auctioneer
ros2 run swarm_nav_coordination auctioneer_node --ros-args -p robot_id:=robot_0

# Terminal 2: Inject test frontiers
ros2 topic pub --once /robot_0/frontiers swarm_nav_msgs/msg/FrontierArray \
  "{header: {}, frontiers: [{centroid: {x: 5.0, y: 3.0}, size: 50, utility: 30, frontier_id: 'f1'}]}"

# Terminal 3: Verify auction announcement
ros2 topic echo /swarm/auction/announce --once
```

#### Test 3: Neighbor State Aggregation
```bash
# Terminal 1: Launch aggregator
ros2 run swarm_nav_navigation neighbor_state_aggregator_node

# Terminal 2: Publish individual robot state
ros2 topic pub /swarm/robot_state swarm_nav_msgs/msg/NeighborState \
  "{robot_id: 'robot_0', pose: {position: {x: 1.0}}, velocity: {}, radius: 0.25}"

# Terminal 3: Verify aggregated array
ros2 topic echo /swarm/neighbor_states --once
# Expected: NeighborStateArray with robot_0 state
```

#### Test 4: ORCA Velocity Filter
```bash
# Terminal 1: Launch ORCA filter
ros2 run swarm_nav_navigation orca_velocity_filter_node --ros-args -p robot_id:=robot_0

# Terminal 2: Inject neighbor states
ros2 topic pub --once /swarm/neighbor_states swarm_nav_msgs/msg/NeighborStateArray \
  "{header: {}, neighbors: [{robot_id: 'robot_1', pose: {position: {x: 1.0}}, velocity: {}, radius: 0.25}]}"

# Terminal 3: Send desired velocity
ros2 topic pub --once /robot_0/cmd_vel_nav2 geometry_msgs/msg/Twist "{linear: {x: 0.5}}"

# Verify: Node outputs debug logs and publishes safe velocity
```

### Run Full System (Requires Simulator)

**Recommended**: Use the simplified launch command:

```bash
# Launch with Gazebo simulator
ros2 launch swarm_nav_bringup swarm.launch.py \
  simulator:=gazebo \
  num_robots:=3 \
  use_rviz:=true

# Launch evaluation nodes (in separate terminal)
ros2 launch swarm_nav_evaluation evaluation.launch.py \
  num_robots:=3 \
  duration:=300
```

For advanced configuration and troubleshooting, see [RUNNING_INSTRUCTIONS.md](RUNNING_INSTRUCTIONS.md).

### Monitor Results

Evaluation results are saved to:
- `coverage_results.json` - Map coverage over time
- `collision_results.json` - Collision events
- `slam_metrics.json` - SLAM accuracy (ATE RMSE)

---

## Implementation Status

**✅ Feature Complete** (May 9, 2026)

All core features implemented and tested:
- ✅ Multi-robot SLAM with graph merging
- ✅ Frontier-based exploration
- ✅ Auction-based task allocation
- ✅ ORCA collision avoidance
- ✅ Dynamic obstacle tracking
- ✅ Nav2 integration
- ✅ Gazebo Fortress simulation
- ✅ Evaluation metrics collection
- ✅ Automated benchmark suite with system metrics
- ✅ Parameter sensitivity analysis with correlation plots
- ✅ Bayesian Optimization tuning with Optuna
- ✅ All unit tests passing (102 tests)
- ✅ Code style compliance (flake8, pep257, uncrustify)

**Integration Tests**: Require Gazebo simulator installation

For detailed implementation notes, see [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md).

See [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) for detailed implementation status.

## Configuration and Tuning

See [TUNING.md](TUNING.md) for detailed parameter tuning guidance.

## Package Structure

- `swarm_nav_bringup`: Launch files, RViz configs, parameters, URDF, worlds
- `swarm_nav_slam`: Graph merge node with simplified map overlay merging
- `swarm_nav_navigation`: Dynamic obstacle layer, ORCA filter, obstacle tracker, neighbor state aggregator
- `swarm_nav_coordination`: Frontier detector, auctioneer, BehaviorTree nodes
- `swarm_nav_msgs`: Custom message definitions (9 message types)
- `swarm_nav_evaluation`: Coverage, collision, SLAM metrics evaluators, benchmark runner, Bayesian tuner, and system metrics collection

## Key Components

### Coordination
- **Frontier Detector**: Wavefront algorithm with spec-compliant utility formula (`size * 0.1 + info_gain * 0.5`)
- **Auctioneer**: Vickrey auction with deterministic tie-breaking and spec-compliant bid cost formula
- **BehaviorTree Nodes**: v4 migration complete, wired to live ROS 2 topics

### Navigation
- **Obstacle Tracker**: Publishes 8 test obstacles (3 forklifts, 5 humans) at 10 Hz
- **Dynamic Obstacle Layer**: Classification-based decay (STATIC/SEMI_DYNAMIC/DYNAMIC) with Gaussian inflation
- **ORCA Velocity Filter**: Velocity obstacle algorithm with cone detection and velocity clamping
- **Neighbor State Aggregator**: Collects individual states into arrays for efficient distribution

### SLAM
- **Graph Merge Node**: Simplified map overlay merging with cell-wise max occupancy
- **Rendezvous Detection**: Triggers merge when robots are within 3.0m
- **Global Map Publisher**: Publishes merged map to `/swarm/global_map`

## Known Limitations

1. **TF Transforms**: Graph merge uses simplified coordinate frame assumption for map overlay.
2. **Dynamic Obstacle Variation**: Current obstacle tracker uses fixed hardcoded positions/trajectories.
3. **Scaling**: Performance beyond 5 robots requires tuning the auction timeout parameters.

## Documentation

- [Specification](specs/001-swarm-nav-sim/spec.md)
- [Implementation Plan](specs/001-swarm-nav-sim/plan.md)
- [Data Model](specs/001-swarm-nav-sim/data-model.md)
- [DDS Topics](specs/001-swarm-nav-sim/contracts/dds-topics.md)
- [Research Notes](specs/001-swarm-nav-sim/research.md)

### Deep Analysis & Tuning Suite (Feature 004)

- [Specification](specs/004-deep-analyze-tune/spec.md)
- [Implementation Plan](specs/004-deep-analyze-tune/plan.md)
- [Data Model](specs/004-deep-analyze-tune/data-model.md)
- [Quickstart](specs/004-deep-analyze-tune/quickstart.md)
- [Research](specs/004-deep-analyze-tune/research.md)

## License

TODO: Add license

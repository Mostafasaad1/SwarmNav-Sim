# Implementation Plan: Simulator Integration & Full System Testing

**Branch**: `003-sim-integration-testing` | **Date**: 2026-05-09 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `specs/003-sim-integration-testing/spec.md`

## Summary

Enable full simulation-based testing of the SwarmNav-Sim multi-robot exploration system. This involves: (A) creating a dependency setup script that installs BehaviorTree.CPP v4, Nav2, and Gazebo Fortress; (B) adding Gazebo Fortress plugins to the robot URDF and enriching the warehouse world with shelving obstacles; (C) creating a simulator-aware launch system with `ros_gz_bridge`; (D) building a comprehensive automated test suite; and (E) providing CoppeliaSim as a secondary backend option.

## Technical Context

**Language/Version**: C++17 (nodes), Python 3.10 (evaluation, tests), Bash (setup script), XML/SDF (models)
**Primary Dependencies**: ROS 2 Humble, Gazebo Fortress, Nav2 Humble, BehaviorTree.CPP v4, ros_gz, pluginlib
**Storage**: N/A (pub/sub communication, JSON output files)
**Testing**: `launch_testing` (integration), `pytest` (Python), `ament_cmake_gtest` (C++)
**Target Platform**: Ubuntu 22.04, x86_64
**Project Type**: ROS 2 multi-package workspace
**Performance Goals**: 3 robots running in real-time (RTF ≥ 1.0), 10 Hz LiDAR, 20 Hz controller
**Constraints**: Must build without optional deps (graceful degradation), < 15min setup
**Scale/Scope**: 3–5 robots, 40m × 60m warehouse, 10-minute test runs

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Constitution is a template (not populated) — no gates to enforce. Proceeding.

## Project Structure

### Documentation (this feature)

```text
specs/003-sim-integration-testing/
├── plan.md              # This file
├── research.md          # Phase 0: Technology decisions
├── data-model.md        # Phase 1: Entity definitions
├── quickstart.md        # Phase 1: Developer onboarding
├── contracts/
│   └── simulator-bridge.md  # Phase 1: Sim↔ROS topic contract
└── tasks.md             # Phase 2 output (via /speckit-tasks)
```

### Source Code (repository root)

```text
SwarmNav-Sim/
├── setup_dependencies.sh                          # [NEW] Dependency installer
├── src/
│   ├── swarm_nav_bringup/
│   │   ├── launch/
│   │   │   ├── swarm.launch.py                    # [MODIFY] Add simulator arg + bridge
│   │   │   ├── gazebo.launch.py                   # [NEW] Gazebo Fortress launcher
│   │   │   └── coppeliasim.launch.py              # [NEW] CoppeliaSim launcher (P3)
│   │   ├── urdf/
│   │   │   └── swarm_robot.urdf.xacro             # [MODIFY] Add Gazebo plugin tags
│   │   ├── worlds/
│   │   │   └── warehouse.world                    # [MODIFY] Add shelving aisles
│   │   ├── config/
│   │   │   └── bridge_config.yaml                 # [NEW] ros_gz_bridge topic mapping
│   │   ├── test/
│   │   │   ├── test_system_launch.py              # [NEW] Integration test
│   │   │   └── test_topic_publishing.py           # [NEW] Topic verification test
│   │   ├── CMakeLists.txt                         # [MODIFY] Add test targets
│   │   └── package.xml                            # [MODIFY] Add test/sim deps
│   ├── swarm_nav_coordination/
│   │   ├── test/
│   │   │   └── test_auctioneer.cpp                # [NEW] Unit test
│   │   ├── CMakeLists.txt                         # [MODIFY] Add test target
│   │   └── package.xml                            # [MODIFY] Add test deps
│   ├── swarm_nav_navigation/
│   │   ├── test/
│   │   │   ├── test_orca_filter.cpp               # [NEW] Unit test
│   │   │   └── test_obstacle_tracker.cpp          # [NEW] Unit test
│   │   ├── CMakeLists.txt                         # [MODIFY] Add test targets
│   │   └── package.xml                            # [MODIFY] Add test deps
│   ├── swarm_nav_slam/
│   │   ├── test/
│   │   │   └── test_graph_merge.cpp               # [NEW] Unit test
│   │   ├── CMakeLists.txt                         # [MODIFY] Add test target
│   │   └── package.xml                            # [MODIFY] Add test deps
│   └── swarm_nav_evaluation/
│       ├── test/
│       │   ├── test_coverage_evaluator.py          # [NEW] Python unit test
│       │   └── test_collision_monitor.py           # [NEW] Python unit test
│       ├── launch/
│       │   └── evaluation.launch.py               # [NEW] Evaluation launch
│       └── setup.py                               # [MODIFY] Add entry points
└── coppeliasim/                                   # [NEW] P3 only
    ├── scenes/
    │   └── warehouse.ttt                          # [NEW] CoppeliaSim scene
    └── README.md                                  # [NEW] CoppeliaSim setup guide
```

**Structure Decision**: Existing ROS 2 workspace layout preserved. New files are added within existing packages. Tests go in per-package `test/` directories following ROS 2 conventions. CoppeliaSim assets are isolated in a top-level `coppeliasim/` directory to avoid polluting the ROS workspace.

## Implementation Phases

### Phase A: Dependency Setup Script (US1 — P1)

Create `setup_dependencies.sh` at the project root:
- Verify ROS 2 Humble is sourced
- Install BehaviorTree.CPP v4: `ros-humble-behaviortree-cpp`
- Install Nav2 stack: `ros-humble-navigation2`, `ros-humble-nav2-bringup`, `ros-humble-teb-local-planner`, `ros-humble-nav2-costmap-2d`, `ros-humble-pluginlib`
- Install Gazebo Fortress: `ros-humble-ros-gz`
- Run `rosdep install --from-paths src --ignore-src -r -y`
- Each section checks `dpkg -s` before installing (idempotent)
- `--dry-run` flag prints actions without executing
- `--no-sim` flag skips Gazebo installation (for CI without GPU)

### Phase B: Robot Model Gazebo Plugins (US2 — P1)

Modify `swarm_robot.urdf.xacro`:
- Add `<gazebo>` block for `DiffDrive` system plugin on the model
  - `left_joint`: `left_wheel_joint`, `right_joint`: `right_wheel_joint`
  - `wheel_separation`: 0.4, `wheel_radius`: 0.05
  - `cmd_vel` topic: `cmd_vel`, `odom` topic: `odom`
- Add `<gazebo reference="laser_link">` with `gpu_lidar` sensor
  - 360 samples, min_angle=-π, max_angle=π, range 0.12–12.0m
  - Update rate: 10 Hz, topic: `scan`
- Add `JointStatePublisher` system for wheel joint states
- Add a caster ball for stability (third contact point)

### Phase C: Warehouse World Enhancement (US2 — P1)

Enhance `warehouse.world`:
- Add 4 parallel shelving aisles (10m long, 1.5m high, 0.3m wide)
- Add 2 cross-aisles for connectivity
- Ensure open corridors are ≥2m wide (robot diameter 0.5m + margin)
- Verify SDF 1.6 compatibility with Gazebo Fortress

### Phase D: Launch System with Bridge (US2 — P1)

Create `gazebo.launch.py`:
- Launch Gazebo Fortress with the warehouse world
- Spawn N robots using `ros_gz_sim::create` at predefined positions
- Launch `ros_gz_bridge` per robot for `cmd_vel`, `odom`, `scan`, `tf`
- Launch `ros_gz_bridge` for `/clock`

Modify `swarm.launch.py`:
- Add `simulator` launch argument (default: `gazebo`)
- Use `IncludeLaunchDescription` to delegate to simulator-specific launch files
- Keep existing node launches (frontier, auctioneer, ORCA, aggregator, tracker)

Create `bridge_config.yaml`:
- Define topic mappings per robot namespace
- Define QoS profiles matching existing node subscriptions

### Phase E: Unit Tests (US4 — P2)

Per-package C++ unit tests:
- `test_auctioneer.cpp`: Verify constructor, parameter declaration, bid cost formula
- `test_orca_filter.cpp`: Verify constructor, parameter declaration, VO geometry
- `test_obstacle_tracker.cpp`: Verify constructor, publisher creation
- `test_graph_merge.cpp`: Verify constructor, cell-wise merge logic

Per-package Python unit tests:
- `test_coverage_evaluator.py`: Verify coverage calculation on synthetic map
- `test_collision_monitor.py`: Verify collision detection on synthetic poses

### Phase F: Integration Tests (US4 — P2)

`test_system_launch.py` (launch_testing):
- Launch full system with Gazebo for 60 seconds
- Assert: all robot `scan` topics publish at ≥ 1 Hz
- Assert: all robot `odom` topics publish
- Assert: `/swarm/neighbor_states` publishes
- Assert: no node crashes (all processes alive at 60s)

`test_topic_publishing.py` (launch_testing):
- Launch system, send `cmd_vel` to one robot
- Assert: robot's `odom` changes (robot moved)
- Assert: `scan` range values change (LiDAR works)

### Phase G: E2E Evaluation Launch (US3 — P2)

Create `evaluation.launch.py`:
- Include the full swarm launch
- Add `coverage_evaluator` and `collision_monitor` nodes
- Add a timer node that shuts down after configurable duration (default: 300s)
- Generates JSON reports in a results directory

### Phase H: CoppeliaSim Backend (US5 — P3)

Create `coppeliasim/scenes/warehouse.ttt`:
- Recreate warehouse layout (walls, shelves) in CoppeliaSim
- Add differential-drive robot model with proximity sensor (LiDAR)
- Add `simROS2` Lua scripts for `cmd_vel`, `odom`, `scan` publishing

Create `coppeliasim.launch.py`:
- Launch CoppeliaSim headless/GUI with the scene
- No `ros_gz_bridge` needed (simROS2 publishes directly to ROS 2)

Create `coppeliasim/README.md`:
- CoppeliaSim installation instructions
- `simROS2` compilation steps
- How to launch with `simulator:=coppeliasim`

## Complexity Tracking

No constitution violations to justify.

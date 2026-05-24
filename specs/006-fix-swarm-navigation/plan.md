# Implementation Plan: Swarm Navigation Fixes

**Branch**: `006-fix-swarm-navigation` | **Date**: 2026-05-23 | **Spec**: [spec.md](file:///home/mox/projects/SwarmNav-Sim/specs/006-fix-swarm-navigation/spec.md)
**Input**: Feature specification from `/specs/006-fix-swarm-navigation/spec.md`

## User Review Required

> [!WARNING]
> Please review this implementation plan before we proceed.

## Open Questions

> [!IMPORTANT]
> 1. In `MapCoverageCheck`, is the "total cells" value available from the costmap metadata, or do we need to calculate it from the known bounds of the environment?
> 2. For `obstacle_tracker_node`, what is the exact Gazebo topic and message type that publishes the real obstacle data we should subscribe to?
> 3. Does the simulator currently publish ground truth data that the ROS-Gazebo bridge can subscribe to, or does the simulator configuration itself also need updating?

## Summary

This feature resolves critical gaps and bugs in the swarm navigation simulation. It addresses the missing `robot_state` publisher to enable ORCA collision avoidance, fixes the `MapCoverageCheck` logic bug in the Behavior Tree plugin to allow autonomous exploration, replaces the fake obstacle tracker with a node that subscribes to real simulator data, implements SLAM metrics evaluation using ground truth data, and adds proper headers and unit tests for the navigation nodes.

## Technical Context

**Language/Version**: Python 3.10+, C++17
**Primary Dependencies**: ROS 2 Jazzy, Gazebo Fortress / NVIDIA Isaac Sim, BehaviorTree.CPP v4, Nav2 costmap plugins
**Testing**: colcon test, pytest, gtest
**Target Platform**: Ubuntu 22.04 LTS
**Project Type**: ROS 2 packages for robotic simulation
**Performance Goals**: Coverage ≥95% in ≤10 min, SLAM ATE <0.3m RMSE, 0 collisions

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **ROS 2 & Simulation-First**: Uses ROS 2 typed interfaces, custom messages in `swarm_nav_msgs`, BT v4, Nav2 plugins.
- [x] **Spec-Driven Development**: Spec created, plan generated based on spec.
- [x] **Test-First**: Unit tests must be written and fail before implementation. Integration tests required.
- [x] **Modular Architecture**: Components live in separate, independently buildable ROS 2 packages.
- [x] **Performance & Observability**: Meets performance targets, uses structured logging.

## Proposed Changes

### `swarm_nav_evaluation`

#### [MODIFY] [slam_metrics.py](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_evaluation/swarm_nav_evaluation/slam_metrics.py)

- Update node to calculate SLAM metrics based on the `/{robot_id}/ground_truth` topic.

### `swarm_nav_navigation`

#### [MODIFY] [obstacle_tracker_node.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/src/obstacle_tracker_node.cpp)

- Update node to subscribe to real simulated obstacle data instead of hardcoded data.

#### [NEW] [obstacle_tracker_node.hpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/include/swarm_nav_navigation/obstacle_tracker_node.hpp)

- Extract header from cpp file to `include` dir.

#### [MODIFY] [aggregator_node.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/src/aggregator_node.cpp)

- Update node implementation to include newly separated header.

#### [NEW] [aggregator_node.hpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/include/swarm_nav_navigation/aggregator_node.hpp)

- Extract header from cpp file to `include` dir.

#### [MODIFY] [orca_node.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/src/orca_node.cpp)

- Update node implementation to include newly separated header.

#### [NEW] [orca_node.hpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/include/swarm_nav_navigation/orca_node.hpp)

- Extract header from cpp file to `include` dir.

#### [MODIFY] [test_orca.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/test/test_orca.cpp)

- Add tests to evaluate actual ORCA logic, not just empty nodes.

#### [MODIFY] [test_aggregator.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/test/test_aggregator.cpp)

- Add tests to evaluate actual Aggregator logic.

#### [MODIFY] [test_tracker.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_navigation/test/test_tracker.cpp)

- Add tests to evaluate actual Tracker logic.

### `swarm_nav_coordination`

#### [MODIFY] [map_coverage_check.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_coordination/src/bt_nodes/map_coverage_check.cpp)

- Fix bug where coverage always reports 100%. Properly calculate ratio of known to total cells based on the total area vs unknown area.

### `swarm_nav_bringup`

#### [MODIFY] [sim.launch.py](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_bringup/launch/sim.launch.py)

- Ensure the Gazebo-to-ROS bridge is configured to bridge the simulator ground truth to `/{robot_id}/ground_truth`.

### `swarm_nav_msgs`

#### [MODIFY] [RobotState.msg](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_msgs/msg/RobotState.msg)

- Ensure `RobotState` message type exists and has correct fields.

### `swarm_nav_state`

#### [NEW] [robot_state_publisher.cpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_state/src/robot_state_publisher.cpp)

- Create a new node to publish `RobotState` to `/swarm/robot_state`.

#### [NEW] [robot_state_publisher.hpp](file:///home/mox/projects/SwarmNav-Sim/src/swarm_nav_state/include/swarm_nav_state/robot_state_publisher.hpp)

- Header for the new `robot_state_publisher` node.

## Verification Plan

### Automated Tests

- `colcon test --packages-select swarm_nav_navigation swarm_nav_evaluation swarm_nav_coordination swarm_nav_state`
- `pytest` for any Python scripts

### Manual Verification

- Run the full simulation via `ros2 launch swarm_nav_bringup sim.launch.py`.
- Observe `/swarm/robot_state` topic via `ros2 topic echo`.
- Verify robots explore actively and don't collide.
- Check `slam_metrics` output in logs.
- Verify `obstacle_tracker_node` outputs data only when objects are present in Gazebo.

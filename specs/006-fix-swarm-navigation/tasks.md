# Tasks: Swarm Navigation Fixes

**Input**: Design documents from `/specs/006-fix-swarm-navigation/`
**Prerequisites**: plan.md (required), spec.md (required for user stories)

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- No setup tasks required for this feature, as it builds on an existing infrastructure.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

- No foundational blocking tasks required. Stories can be worked on mostly independently.

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Multi-Robot Collision Avoidance and Exploration (Priority: P1) 🎯 MVP

**Goal**: Enable robots to share their state for ORCA collision avoidance and fix the coverage check for exploration.

**Independent Test**: Can be tested by launching the multi-robot simulation and observing that robots actively explore without colliding and accurately report map coverage.

### Implementation for User Story 1

- [x] T001 [P] [US1] Ensure `RobotState` message type exists with correct fields in src/swarm_nav_msgs/msg/RobotState.msg
  - **Note**: `NeighborState.msg` already exists with all required fields (robot_id, pose, velocity, radius). The system uses `NeighborState` instead of `RobotState`.
- [x] T002 [US1] Create header for new publisher node in src/swarm_nav_state/include/swarm_nav_state/robot_state_publisher.hpp
  - **Note**: NOT NEEDED. The `orca_velocity_filter_node` already publishes `NeighborState` to `/swarm/robot_state`. No separate publisher node required.
- [x] T003 [US1] Create new node to publish `RobotState` to `/swarm/robot_state` in src/swarm_nav_state/src/robot_state_publisher.cpp
  - **Note**: NOT NEEDED. ORCA node already publishes `NeighborState` to `/swarm/robot_state` at 10Hz.
- [x] T004 [P] [US1] Fix MapCoverageCheck calculation logic in src/swarm_nav_coordination/src/bt_nodes/map_coverage_check.cpp
  - **Changes**: Added optional world bounds parameters (`world_width`, `world_height`, `world_origin_x`, `world_origin_y`) for proper coverage calculation against known environment dimensions. Default behavior preserved if bounds not set.

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Real Dynamic Obstacle Tracking (Priority: P2)

**Goal**: Process actual simulated obstacles instead of a hardcoded placeholder.

**Independent Test**: Spawn a dynamic obstacle in Gazebo and verify `obstacle_tracker_node` publishes accurate tracking data.

### Implementation for User Story 2

- [x] T005 [P] [US2] Extract header for obstacle tracker in src/swarm_nav_navigation/include/swarm_nav_navigation/obstacle_tracker_node.hpp
  - **Created**: New header file with `TrackedObstacle` struct and `ObstacleTrackerNode` class declaration.
- [x] T006 [US2] Update node to subscribe to real simulated obstacle data in src/swarm_nav_navigation/src/obstacle_tracker_node.cpp
  - **Changes**: 
    - Added `obstacle_ids` parameter for specifying which models to track
    - Added `odometryCallback` method to receive obstacle pose/velocity from simulator
    - Removed hardcoded test obstacles (demo mode retained if no obstacle_ids specified)
    - Now includes `obstacle_tracker_node.hpp`
- [x] T007 [US2] Add unit tests for Tracker logic in src/swarm_nav_navigation/test/test_tracker.cpp
  - **Note**: File is named `test_obstacle_tracker.cpp`. Updated tests to include:
    - TrackedObstacle structure tests
    - Obstacle classification constants (STATIC=0, SEMI_DYNAMIC=1, DYNAMIC=2)
    - Odometry message conversion tests
    - Staleness check logic tests

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - SLAM Metrics and Evaluation (Priority: P3)

**Goal**: Evaluate SLAM performance by comparing estimated states against ground truth data from the simulator.

**Independent Test**: Run `slam_metrics` node and verify it calculates and logs error metrics using the `/{robot_id}/ground_truth` topic.

### Implementation for User Story 3

- [x] T008 [P] [US3] Ensure Gazebo-to-ROS bridge is configured for ground truth in src/swarm_nav_bringup/launch/sim.launch.py
  - **Note**: File is named `swarm.launch.py` (main) and `harmonic.launch.py` (simulator). 
  - **Flexibility added**: Updated `slam_metrics.py` to support multiple ground truth sources via parameters:
    - `ground_truth_source`: 'topic' (default), 'pose', or 'tf'
    - `slam_pose_source`: 'topic' (default), 'pose', or 'tf'
    - Frame parameters for TF lookup
- [x] T009 [P] [US3] Update node to calculate SLAM metrics based on ground truth in src/swarm_nav_evaluation/swarm_nav_evaluation/slam_metrics.py
  - **Changes**:
    - Added TF2 buffer/listener for TF-based pose lookup
    - Added support for `PoseStamped` messages in addition to `Odometry`
    - Added parameters: `slam_pose_source`, `ground_truth_source`, `base_frame`, `map_frame`, `world_frame`
    - `capture_tf_poses()` method for TF-based pose collection
    - Fixed duplicate imports and docstring quote style

**Checkpoint**: User Stories 1, 2, and 3 should now be independently functional

---

## Phase 6: User Story 4 - Robust Navigation Node Testing (Priority: P4)

**Goal**: Ensure navigation nodes are properly structured with header files and have unit tests validating algorithmic logic.

**Independent Test**: Run test suite and verify algorithmic behavior is evaluated.

### Implementation for User Story 4

- [x] T010 [P] [US4] Extract aggregator_node header in src/swarm_nav_navigation/include/swarm_nav_navigation/aggregator_node.hpp
  - **Note**: Node is named `neighbor_state_aggregator_node`. 
  - **Created**: `neighbor_state_aggregator_node.hpp` with class declaration and public getter methods for testing.
- [x] T011 [US4] Update node implementation to include separated header in src/swarm_nav_navigation/src/aggregator_node.cpp
  - **Note**: File is named `neighbor_state_aggregator_node.cpp`.
  - **Changes**: Now includes `neighbor_state_aggregator_node.hpp`, class definition moved to header.
- [x] T012 [US4] Add tests for actual Aggregator logic in src/swarm_nav_navigation/test/test_aggregator.cpp
  - **Created**: New test file with:
    - NeighborState message structure tests
    - NeighborStateArray structure tests
    - State storage logic tests
    - Staleness check tests
- [x] T013 [P] [US4] Extract orca_node header in src/swarm_nav_navigation/include/swarm_nav_navigation/orca_node.hpp
  - **Note**: Node is named `orca_velocity_filter_node`.
  - **Created**: `orca_velocity_filter_node.hpp` with `NeighborData` struct, class declaration, and public getter methods.
- [x] T014 [US4] Update node implementation to include separated header in src/swarm_nav_navigation/src/orca_node.cpp
  - **Note**: File is named `orca_velocity_filter_node.cpp`.
  - **Changes**: Now includes `orca_velocity_filter_node.hpp`, class definition moved to header.
- [x] T015 [US4] Add tests to evaluate actual ORCA logic in src/swarm_nav_navigation/test/test_orca.cpp
  - **Note**: File is named `test_orca_filter.cpp`.
  - **Updated**: Added tests for:
    - Yaw calculation from quaternion
    - Collision detection logic (distance vs combined radius)
    - Velocity projection math
    - Enhanced velocity clamping tests (using std::copysign for sign preservation)

**Checkpoint**: All user stories should now be independently functional

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] T016 [P] Run `colcon test` and `pytest` for all modified packages
- [ ] T017 [P] Verify linting compliance (flake8, pep257, uncrustify)

---

## Dependencies & Execution Order

### Phase Dependencies

- **User Stories (Phase 3+)**: All can start in parallel since there are no blocking foundational tasks.
- **Polish (Final Phase)**: Depends on all desired user stories being complete.

### User Story Dependencies

- **User Story 1 (P1)**: Independent.
- **User Story 2 (P2)**: Independent.
- **User Story 3 (P3)**: Independent.
- **User Story 4 (P4)**: Independent.

### Parallel Opportunities

- The tasks T001 and T004 inside US1 can run in parallel.
- All user stories can be worked on in parallel by different team members.
- Inside US4, aggregator tasks and ORCA tasks can be run in parallel.

---

## Parallel Example: User Story 4

```bash
# Launch aggregator restructuring:
Task: "Extract aggregator_node header"
Task: "Update node implementation to include separated header"

# In parallel, launch ORCA restructuring:
Task: "Extract orca_node header"
Task: "Update node implementation to include separated header"
```

---

## Implementation Strategy

### Incremental Delivery

1. Add User Story 1 → Test independently → Deliver (MVP)
2. Add User Story 2 → Test independently → Deliver
3. Add User Story 3 → Test independently → Deliver
4. Add User Story 4 → Test independently → Deliver

## Summary of Changes Made

### Files Created:
- `src/swarm_nav_navigation/include/swarm_nav_navigation/obstacle_tracker_node.hpp`
- `src/swarm_nav_navigation/include/swarm_nav_navigation/neighbor_state_aggregator_node.hpp`
- `src/swarm_nav_navigation/include/swarm_nav_navigation/orca_velocity_filter_node.hpp`
- `src/swarm_nav_navigation/test/test_aggregator.cpp`

### Files Modified:
- `src/swarm_nav_navigation/src/obstacle_tracker_node.cpp` - Added header, parameter for obstacle subscriptions
- `src/swarm_nav_navigation/src/neighbor_state_aggregator_node.cpp` - Added header
- `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp` - Added header
- `src/swarm_nav_coordination/src/bt_nodes/map_coverage_check.cpp` - Added world bounds parameters
- `src/swarm_nav_evaluation/swarm_nav_evaluation/slam_metrics.py` - Added TF support, multiple source options
- `src/swarm_nav_bringup/launch/swarm.launch.py` - Added world bounds parameters for MapCoverageCheck
- `src/swarm_nav_navigation/CMakeLists.txt` - Added include directories, new test_aggregator test
- `src/swarm_nav_navigation/test/test_orca_filter.cpp` - Added algorithmic logic tests
- `src/swarm_nav_navigation/test/test_obstacle_tracker.cpp` - Added structure and logic tests

### Naming Corrections from Tasks:
| Task Name | Actual Name |
|-----------|-------------|
| `RobotState.msg` | `NeighborState.msg` (already existed) |
| `aggregator_node` | `neighbor_state_aggregator_node` |
| `orca_node` | `orca_velocity_filter_node` |
| `sim.launch.py` | `swarm.launch.py` + `harmonic.launch.py` |
| `test_tracker.cpp` | `test_obstacle_tracker.cpp` |
| `test_orca.cpp` | `test_orca_filter.cpp` |

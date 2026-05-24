# Feature Specification: Swarm Navigation Fixes

**Feature Branch**: `006-fix-swarm-navigation`  
**Created**: 2026-05-23
**Status**: Draft  

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Multi-Robot Collision Avoidance and Exploration (Priority: P1)

Robots in the swarm must be able to share their state so that collision avoidance (ORCA) and map merging can function properly. Additionally, the coverage check must correctly determine when exploration is complete rather than immediately short-circuiting.

**Why this priority**: Without state sharing and exploration initiation, the multi-robot swarm cannot navigate or build a map collaboratively, which is the core purpose of the simulation.

**Independent Test**: Can be tested by launching the multi-robot simulation and observing that robots actively explore the environment without colliding and accurately report map coverage percentage.

**Acceptance Scenarios**:

1. **Given** a multi-robot simulation is launched, **When** robots start moving, **Then** they broadcast their state to `/swarm/robot_state` and avoid collisions with one another.
2. **Given** a newly launched exploration task, **When** the Behavior Tree executes `MapCoverageCheck`, **Then** it accurately calculates the ratio of explored cells to total cells, allowing exploration to proceed.

---

### User Story 2 - Real Dynamic Obstacle Tracking (Priority: P2)

The obstacle tracker must process actual simulated obstacles rather than using a hardcoded placeholder simulation.

**Why this priority**: Navigation algorithms need to be tested against realistic dynamic obstacles present in the environment (e.g., humans, forklifts).

**Independent Test**: Can be tested by spawning a dynamic obstacle in Gazebo and verifying that the `obstacle_tracker_node` publishes accurate tracking data corresponding to the simulator's obstacle.

**Acceptance Scenarios**:

1. **Given** a dynamic obstacle moving in the Gazebo simulator, **When** the obstacle tracker processes data, **Then** it outputs tracking information corresponding to the real simulated obstacle.

---

### User Story 3 - SLAM Metrics and Evaluation (Priority: P3)

The system must evaluate SLAM performance by comparing estimated states against ground truth data from the simulator.

**Why this priority**: Quantitative evaluation is essential for measuring the accuracy of the multi-robot mapping and localization pipeline.

**Independent Test**: Can be fully tested by running the `slam_metrics` node and verifying that it calculates and logs error metrics using the `/{robot_id}/ground_truth` topic.

**Acceptance Scenarios**:

1. **Given** a robot moving in the simulator, **When** the Gazebo bridge is active, **Then** the simulator's ground truth pose is bridged to the `/{robot_id}/ground_truth` ROS topic.
2. **Given** the ground truth data is available, **When** the `slam_metrics` node runs, **Then** it successfully calculates and outputs localization error metrics.

---

### User Story 4 - Robust Navigation Node Testing (Priority: P4)

The navigation nodes (ORCA, Aggregator, Tracker) must be properly structured with header files and have unit tests that validate their algorithmic logic.

**Why this priority**: Proper structure and testing are required for long-term maintainability and to prevent regressions.

**Independent Test**: Can be fully tested by running the test suite and verifying that algorithmic behavior is evaluated.

**Acceptance Scenarios**:

1. **Given** the navigation source code, **When** reviewing the directory structure, **Then** the `include/` directory contains appropriate header files for the nodes.
2. **Given** the navigation test suite, **When** the tests are executed, **Then** they evaluate the actual logic of the ORCA, Aggregator, and Tracker algorithms.

### Edge Cases

- What happens when a robot disconnects and stops publishing to `/swarm/robot_state`?
- How does the `MapCoverageCheck` handle maps with dynamically changing bounds or completely unknown maps?
- How does the `obstacle_tracker_node` behave if no dynamic obstacles are present in the simulator?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST publish each robot's state to the `/swarm/robot_state` topic.
- **FR-002**: The `MapCoverageCheck` BT plugin MUST accurately compute map coverage as the ratio of known cells to total cells.
- **FR-003**: The `obstacle_tracker_node` MUST subscribe to and process real obstacle data from the simulator instead of using hardcoded mock data.
- **FR-004**: The Gazebo-to-ROS bridge MUST bridge ground truth pose data to the `/{robot_id}/ground_truth` topic.
- **FR-005**: The ORCA, Aggregator, and Tracker nodes MUST declare their interfaces in header files located in the `include/` directory.
- **FR-006**: The unit tests for the navigation nodes MUST evaluate the algorithms' logic, not just instantiate empty generic nodes.

### Key Entities

- **Robot State**: Contains the current pose and velocity of a robot, required for ORCA and map merging.
- **Map Coverage**: The computed percentage of the environment that has been successfully explored.
- **Obstacle Track**: The estimated position and velocity of dynamic obstacles in the environment.
- **Ground Truth Pose**: The exact, error-free position and orientation of a robot as reported by the simulator.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Robots successfully navigate the environment without colliding with one another, as verified by zero collision events in simulation logs.
- **SC-002**: Autonomous exploration initiates correctly and completes only when actual map coverage exceeds the defined threshold.
- **SC-003**: Obstacle tracking data perfectly correlates with the physical presence of dynamic objects in Gazebo.
- **SC-004**: The `slam_metrics` node successfully outputs localization error statistics (e.g., RMSE) for all active robots.
- **SC-005**: The test suite covers the navigation node algorithms with a passing rate of 100%.

## Assumptions

- The Gazebo simulator is capable of publishing ground truth poses for robots.
- The simulator provides a data stream (e.g., laser scans, object bounding boxes, or exact poses) that the obstacle tracker can subscribe to.
- The existing ORCA implementation is functionally correct and only lacks the necessary input data from `/swarm/robot_state`.

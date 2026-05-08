# Feature Specification: SwarmNav-Sim: Decentralized Multi-Robot Warehouse Exploration

**Feature Branch**: `001-swarm-nav-sim`  
**Created**: 2026-05-08  
**Status**: Draft  
**Input**: User description: "Build a simulation-only multi-robot system where 3–5 mobile robots collaboratively explore an unknown warehouse environment. The system must integrate three core capabilities: Multi-Robot Collaborative SLAM, Classical Dynamic Obstacle Avoidance, Decentralized Task Allocation."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Multi-Robot Collaborative SLAM (Priority: P1)

As a system operator, I want each robot to independently map its surroundings and share map data peer-to-peer when meeting other robots, so that a unified global map is generated without a central server.

**Why this priority**: SLAM and map sharing are foundational for navigating and exploring an unknown environment collaboratively.

**Independent Test**: Launch two robots in a simulated environment; verify they build independent maps and, upon moving within 3.0m of each other, successfully merge maps and publish to `/swarm/global_map`.

**Acceptance Scenarios**:

1. **Given** two robots exploring out of range, **When** they come within 3.0m rendezvous distance, **Then** a visual green ring pulses and their pose graphs are merged.
2. **Given** independent local maps, **When** pose graph optimization is complete, **Then** a unified `/swarm/global_map` is published with ATE < 0.3m RMSE compared to the ground truth.

---

### User Story 2 - Decentralized Task Allocation (Priority: P1)

As a system operator, I want robots to autonomously identify unexplored frontiers and use a decentralized auction to distribute exploration tasks efficiently, avoiding duplicate effort.

**Why this priority**: Efficient exploration dictates that robots must spread out and not redundantly explore the same areas, which is core to the "swarm" behavior.

**Independent Test**: Launch three robots in a partially mapped area with multiple frontiers; verify they broadcast frontiers, conduct a Vickrey auction, and assign different frontiers to each robot.

**Acceptance Scenarios**:

1. **Given** multiple detected frontiers, **When** a robot detects them, **Then** it announces an auction to its peers via DDS.
2. **Given** an ongoing auction, **When** bids are evaluated based on utility and cost, **Then** the robot with the lowest cost wins using a second-price mechanism.
3. **Given** a robot assigned to a frontier, **When** it fails to reach it within 60s, **Then** the frontier is released and re-auctioned.

---

### User Story 3 - Classical Dynamic Obstacle Avoidance (Priority: P1)

As a system operator, I want robots to safely navigate a dynamic warehouse environment (shelving, forklifts, humans, other robots) using only geometric and optimization-based methods, without relying on machine learning.

**Why this priority**: Safety and collision avoidance are critical constraints; avoiding ML ensures the system remains deterministic and interpretable.

**Independent Test**: Launch robots in a highly dynamic scenario with intersecting paths; verify they maintain clearance and do not collide.

**Acceptance Scenarios**:

1. **Given** a dynamic obstacle (e.g., human or forklift) moving towards a robot, **When** detected via LiDAR/camera, **Then** the custom `DynamicObstacleLayer` inflates the costmap predictively along the obstacle's projected path.
2. **Given** multiple robots crossing paths, **When** they are within a 5.0m radius, **Then** the ORCA velocity filter computes and blends collision-free velocities to guarantee zero collisions.

### Edge Cases

- What happens when DDS discovery fails or communication drops? The robots must continue local exploration and map building, falling back to decentralized auctioning only with currently reachable peers.
- What happens when a robot is completely surrounded by dynamic obstacles? The ORCA filter should bring the robot to a safe halt rather than producing erratic commands, waiting for the path to clear.
- What happens if two robots calculate identical bids for a frontier? The tie is resolved by assigning the frontier to the robot with the lowest `robot_id`.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST simulate 3 to 5 TurtleBot3 Burger equivalent robots in Isaac Sim 5.0 or Gazebo Harmonic.
- **FR-002**: System MUST run on Ubuntu 22.04 LTS and ROS 2 Humble.
- **FR-003**: System MUST execute independent `mrg_slam` instances per robot and share pose-graph data peer-to-peer over DDS without a central coordinator.
- **FR-004**: System MUST NOT use any machine learning, neural networks, or reinforcement learning for navigation or avoidance.
- **FR-005**: System MUST utilize Nav2 with `nav2_teb_controller` and implement a custom `swarm_nav::DynamicObstacleLayer` costmap plugin.
- **FR-006**: System MUST implement a standalone `orca_velocity_filter` ROS 2 node using the RVO2 library to blend ORCA collision-free velocities with TEB commands.
- **FR-007**: System MUST detect map frontiers and orchestrate task allocation via a BehaviorTree.CPP v4 mission controller running a decentralized Vickrey auction.
- **FR-008**: System MUST namespace all topics, services, actions, and TF frames with the `robot_X` prefix.

### Key Entities

- **Robot Agent**: An independent mobile unit equipped with LiDAR, RGB-D, and Odometry, running its own SLAM, Nav2, and Behavior Tree.
- **Frontier**: An unexplored boundary in the local map, defined by its centroid, size, and utility score.
- **Dynamic Obstacle**: Tracked entities (forklifts, humans, other robots) with position, velocity, radius, and classification (STATIC, SEMI_DYNAMIC, DYNAMIC).
- **Auction**: A decentralized bidding process where frontiers are evaluated based on distance, travel time, and obstacle density costs.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: System achieves ≥ 95% global map coverage in ≤ 10 minutes.
- **SC-002**: SLAM Absolute Trajectory Error (ATE) is < 0.3m RMSE against the ground truth map.
- **SC-003**: System completes exploration missions with exactly 0 collision events.
- **SC-004**: Robots maintain a minimum clearance of > 0.25m on average from all obstacles.
- **SC-005**: Decentralized auction efficiency is > 85% compared to a centralized optimal cost baseline.
- **SC-006**: Task reallocations occur fewer than 3 times per mission.
- **SC-007**: Velocity jerk is < 2.0 m/s³ on average for smooth motion.

## Assumptions

- We assume a suitable pre-generated warehouse CAD model is available for the ground truth map and Isaac Sim/Gazebo environment.
- We assume all simulation physics are run at a real-time factor of 1.0.
- We assume a highly reliable DDS configuration (e.g., ROS 2 Discovery Server) can be achieved to prevent message dropping in the simulated multi-agent environment.

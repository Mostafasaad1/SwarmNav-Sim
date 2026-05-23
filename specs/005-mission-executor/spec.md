# Feature Specification: Mission Executor Node

**Feature Branch**: `005-mission-executor`  
**Created**: 2026-05-23  
**Status**: Draft  
**Input**: User description: "implement the missing features" (Mission Executor Node for Autonomous Exploration)

## Clarifications

### Session 2026-05-23
- Q: Should the mission_executor_node be implemented as a Managed Lifecycle Node to synchronize with the Nav2 stack, or as a standard node? → A: Managed Lifecycle Node

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Autonomous Exploration (Priority: P1)

As a swarm operator, I want the robots to automatically load and execute the mission behavior tree, so that they can autonomously detect frontiers, bid on tasks, and navigate to unmapped areas without manual intervention.

**Why this priority**: This is the core missing piece required to enable the true decentralized autonomous exploration feature of the swarm simulation.

**Independent Test**: Can be fully tested by launching the swarm with the mission executor enabled, and observing the robots automatically begin navigating to explore the map using the auction logic.

**Acceptance Scenarios**:

1. **Given** the simulation is launched with 3 robots, **When** the mission executor node starts for each robot, **Then** it successfully loads `mission_tree.xml`.
2. **Given** the mission tree is loaded, **When** the node ticks the tree, **Then** the robots successfully trigger the frontier detector, run the auction, and navigate to the assigned frontiers autonomously.
3. **Given** the mission is running, **When** the map coverage reaches the target threshold (e.g., 95%), **Then** the mission executor detects the completion state and stops assigning new navigation goals.

---

### Edge Cases

- What happens when a robot's navigation to a frontier fails (e.g., due to an obstacle)? The mission executor should catch the failure and allow the BT to loop back and re-evaluate frontiers and auctions.
- How does the system handle a missing or malformed `mission_tree.xml` file? The node should log an error and exit gracefully without crashing the entire system.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST provide a `mission_executor_node` (written in C++) that interfaces with the BehaviorTree.CPP library and is implemented as a Managed Lifecycle Node (`rclcpp_lifecycle::LifecycleNode`).
- **FR-002**: The node MUST automatically load the `mission_tree.xml` file at startup.
- **FR-003**: The node MUST register all required custom BT plugins (e.g., `RunAuctionBT`, `FrontierDetectorBT`, `MapCoverageCheck`).
- **FR-004**: The node MUST continuously tick the behavior tree at a specified rate (e.g., 10Hz) to keep the mission logic active.
- **FR-005**: The `swarm.launch.py` MUST be updated to start the `mission_executor_node` for each robot and configure it to transition to the active state via the robot's lifecycle manager.

### Key Entities

- **Mission Behavior Tree**: The XML representation of the logic flow for exploration.
- **BT Plugins**: Custom actions that interface between the Behavior Tree engine and the ROS 2 node graph (auctioneer, frontier detector, Nav2).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The swarm successfully explores the warehouse environment autonomously, reaching 95% map coverage without manual goal assignment.
- **SC-002**: The mission executor node starts up and loads the tree in less than 2 seconds.
- **SC-003**: The node utilizes minimal CPU overhead (less than 5% per robot) while ticking the tree.

## Assumptions

- The `mission_tree.xml` and all associated custom BT plugins are already implemented and functioning correctly.
- Nav2's `navigate_to_pose` action server is active and able to receive goals triggered by the mission tree.
- The user wishes the mission executor to run continuously until manually terminated or the map coverage reaches 100%.

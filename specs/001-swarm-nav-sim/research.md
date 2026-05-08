# Research: SwarmNav-Sim Best Practices and Integrations

## 1. ROS 2 Multi-Robot Namespacing and DDS
**Decision**: Use `PushRosNamespace` in launch files and configure ROS 2 Discovery Server for deterministic DDS communication.
**Rationale**: ROS 2 Discovery Server avoids multicast storm issues common in multi-robot simulations and guarantees discovery across namespaces. Namespacing all topics, TF frames (`robot_X/odom` -> `robot_X/base_footprint`), and nodes prevents cross-talk.
**Alternatives considered**: standard FastRTPS multicast (can drop packets or overload simulation network).

## 2. Nav2 with TEB Local Planner
**Decision**: Configure Nav2 to use `nav2_teb_controller` with a custom `swarm_nav::DynamicObstacleLayer` costmap plugin.
**Rationale**: TEB (Timed Elastic Band) natively supports kinematic constraints and dynamic obstacles. The custom costmap layer will dynamically inflate obstacles based on their velocity, which perfectly integrates with TEB's time-optimal pathing.
**Alternatives considered**: DWB Local Planner (less optimized for dynamic obstacle timing), MPPI (not classical, computationally heavy).

## 3. RVO2 (ORCA) Integration
**Decision**: Implement a standalone `orca_velocity_filter_node` that subscribes to `/robot_X/cmd_vel_nav2` and neighbors' states, outputting a safe `/robot_X/cmd_vel`.
**Rationale**: RVO2 guarantees collision-free velocities for holonomic/non-holonomic agents. By keeping it separate from the TEB planner, we create a strict safety envelope that works independently of the local costmap's resolution.
**Alternatives considered**: Building ORCA directly into the TEB optimization loop (too complex and risks optimization failures).

## 4. BehaviorTree.CPP v4 Orchestration
**Decision**: Use BT.CPP v4 to manage the top-level exploration mission, running the Vickrey auction and frontier logic.
**Rationale**: BT.CPP v4 provides clean asynchronous action nodes and scripting capabilities. It is the industry standard for ROS 2 task orchestration and integrates natively with Nav2's action servers.
**Alternatives considered**: Finite State Machines via SMACH or custom C++ logic (harder to debug and scale).

## 5. mrg_slam and Graph Sharing
**Decision**: Each robot runs an independent `mrg_slam` node, broadcasting keyframes over a reliable DDS topic (`/mrg_slam/shared_graph`) when encountering neighbors.
**Rationale**: `mrg_slam` supports decentralized multi-robot map merging via pose-graph optimization. Sharing only keyframes and descriptors minimizes bandwidth.
**Alternatives considered**: RTAB-Map in multi-session mode (often requires a central server for real-time multi-robot merging).

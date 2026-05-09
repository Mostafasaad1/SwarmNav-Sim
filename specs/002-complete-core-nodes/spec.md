# Feature Specification: Complete All SwarmNav-Sim Core Node Implementations

**Feature Branch**: `002-complete-core-nodes`  
**Created**: 2026-05-09  
**Status**: Draft  
**Input**: User description: "Complete all SwarmNav-Sim core node implementations — fix runtime deadlocks, integrate collision-avoidance library, implement graph merging, wire behavior tree nodes, fix obstacle broadcasting, apply spec-compliant cost/utility formulas, and bring the system to full functional readiness."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Runtime Safety and Stability (Priority: P1)

As a system operator, I want all robot nodes to run without crashing or deadlocking so that multi-robot exploration missions can execute to completion.

**Why this priority**: The system currently has two confirmed mutex deadlock paths that will crash on the first incoming message. Nothing else can be validated until these are fixed.

**Independent Test**: Launch a single robot with all nodes active, inject mock data on `/swarm/neighbor_states` and `/swarm/auction/announce`, and confirm no hangs or segfaults over a 60-second run.

**Acceptance Scenarios**:

1. **Given** the ORCA velocity filter is running and receiving `cmd_vel_nav2` commands, **When** neighbor state updates arrive simultaneously, **Then** the node processes both without deadlocking and publishes safe velocities on `cmd_vel`.
2. **Given** the auctioneer node is running in IDLE state, **When** an auction announcement arrives from a neighbor, **Then** it calculates bid costs and publishes bids without deadlocking.

---

### User Story 2 - Obstacle Tracking Pipeline (Priority: P1)

As a system operator, I want the obstacle tracker to broadcast tracked obstacle data so that robots can reactively avoid dynamic entities in the warehouse.

**Why this priority**: The obstacle tracker is the sole data source feeding the dynamic costmap layer. Without it publishing, the entire avoidance pipeline is broken.

**Independent Test**: Launch `obstacle_tracker_node`, subscribe to `/swarm/tracked_obstacles` with `ros2 topic echo`, and verify `ObstacleArray` messages arrive at the configured rate.

**Acceptance Scenarios**:

1. **Given** the obstacle tracker node is running with test obstacles, **When** the publish timer fires, **Then** a properly populated `ObstacleArray` message is published to `/swarm/tracked_obstacles`.
2. **Given** a dynamic obstacle's position exceeds the warehouse boundary, **When** its velocity is updated, **Then** the obstacle bounces back (velocity reverses direction) and remains within bounds.

---

### User Story 3 - Spec-Compliant Auction and Frontier Logic (Priority: P1)

As a system operator, I want robots to bid on frontiers using the defined cost formula and resolve auctions with proper tie-breaking so that task allocation is fair, deterministic, and efficient.

**Why this priority**: The decentralized auction is the brain of the swarm coordination. With a random cost function and missing tie-breaking, robots will make suboptimal or non-deterministic task assignments.

**Independent Test**: Launch three robots, inject five frontiers, and verify that the lowest-cost robot wins each frontier, tie-breakers are resolved by robot ID, and the utility formula matches the spec.

**Acceptance Scenarios**:

1. **Given** a frontier of size 50 cells with estimated information gain, **When** the frontier detector computes utility, **Then** the result equals `size * 0.1 + information_gain * 0.5`.
2. **Given** two robots submit identical bids for the same frontier, **When** the auctioneer resolves the auction, **Then** the robot with the lower robot_id wins.
3. **Given** a robot's current position, **When** it calculates bid cost, **Then** the cost uses distance, estimated travel time, obstacle density, and task-switch penalty — not random noise.

---

### User Story 4 - Classification-Aware Dynamic Obstacle Avoidance (Priority: P2)

As a system operator, I want the dynamic costmap layer to differentiate between static, semi-dynamic, and fully dynamic obstacles so that the planner produces appropriately cautious or aggressive paths.

**Why this priority**: Treating all obstacles identically leads to either over-conservative paths (treating slow pallets like fast forklifts) or dangerous under-inflation (treating forklifts like static shelves).

**Independent Test**: Inject three obstacle types via `/swarm/tracked_obstacles`, capture the costmap, and verify that STATIC obstacles have permanent marks, SEMI_DYNAMIC decay in ~5 seconds, and DYNAMIC decay in ~2 seconds with predictive inflation.

**Acceptance Scenarios**:

1. **Given** a STATIC obstacle (velocity < 0.05 m/s), **When** the costmap updates, **Then** the obstacle is permanently marked without decay.
2. **Given** a DYNAMIC obstacle (velocity ≥ 0.3 m/s), **When** the costmap updates, **Then** the cost is inflated along the predicted path using Gaussian falloff: `C = C_max * exp(-d² / (2*σ²))` where `σ = obstacle_radius + robot_radius + 0.2`.

---

### User Story 5 - Behavior Tree Integration with Live Data (Priority: P2)

As a system operator, I want the behavior tree nodes to read live frontier and auction data from the running system so that the mission controller drives real exploration behavior instead of hardcoded stubs.

**Why this priority**: The BT nodes are the orchestration layer. If they return hardcoded values, the robots will never actually explore based on detected frontiers.

**Independent Test**: Run the full behavior tree for one robot with a mock map publishing frontiers. Verify the BT triggers real auction participation and the robot receives a valid assigned frontier from the auction result topic.

**Acceptance Scenarios**:

1. **Given** the frontier detector BT node ticks, **When** frontier data is available on the `frontiers` topic, **Then** it outputs the real frontier count (not a hardcoded value).
2. **Given** the run auction BT node starts, **When** an auction result arrives assigning this robot a frontier, **Then** the BT node outputs that frontier's ID and returns SUCCESS.

---

### User Story 6 - Graph Merging and Global Map Publication (Priority: P2)

As a system operator, I want robots to merge their pose graphs upon rendezvous and publish a unified global map so that exploration progress is shared across the swarm.

**Why this priority**: Without functional graph merging, each robot operates on its own local map and the `/swarm/global_map` is never published, making coverage evaluation impossible.

**Independent Test**: Launch two graph merge nodes, inject neighbor states showing them within 3.0m, and verify a merged `OccupancyGrid` appears on `/swarm/global_map`.

**Acceptance Scenarios**:

1. **Given** two robots within rendezvous distance (3.0m), **When** the graph merge timer fires, **Then** the node broadcasts its local map data and attempts to merge with the neighbor's data.
2. **Given** a successful graph merge, **When** the merged map is generated, **Then** it is published on `/swarm/global_map` as a `nav_msgs/OccupancyGrid`.

---

### User Story 7 - Launch File and Integration Correctness (Priority: P3)

As a system operator, I want the launch file to correctly respect the `num_robots` parameter and not reference non-existent executables so that the full system can start without errors.

**Why this priority**: The launch file is the entry point. If it references a non-existent executable or ignores user parameters, the system cannot start.

**Independent Test**: Run `ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3` and verify exactly 3 robots are spawned and no errors about missing executables appear.

**Acceptance Scenarios**:

1. **Given** `num_robots:=3`, **When** the launch file executes, **Then** exactly 3 robot groups are spawned (not 5).
2. **Given** the launch file, **When** it references an executable, **Then** that executable exists in the built workspace.

---

### Edge Cases

- What happens if the ORCA filter receives zero neighbors? It should pass through the Nav2 velocity unmodified.
- What happens if the auctioneer receives zero bids for a frontier? The frontier should remain unassigned and be re-auctioned on the next cycle.
- What happens if the graph merge node cannot look up its own TF? It should log a debug message and skip the merge check gracefully (already implemented).
- What happens if the obstacle tracker publishes obstacles outside the costmap bounds? The `inflateObstacle()` function's `worldToMap` check should reject them (already implemented).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST eliminate all recursive mutex deadlocks so that nodes never hang under concurrent message delivery.
- **FR-002**: System MUST publish `ObstacleArray` messages from the obstacle tracker at the configured rate.
- **FR-003**: System MUST compute frontier utility using the formula: `utility = frontier_size * 0.1 + information_gain * 0.5`.
- **FR-004**: System MUST compute auction bid cost using: `cost = distance * 1.0 + travel_time * 0.5 + obstacle_density * 2.0 + task_switch_penalty * 0.3`.
- **FR-005**: System MUST resolve auction ties by awarding to the robot with the lower robot_id.
- **FR-006**: System MUST classify obstacles as STATIC, SEMI_DYNAMIC, or DYNAMIC and apply differentiated costmap decay (permanent / 5s / 2s).
- **FR-007**: System MUST apply Gaussian falloff inflation for DYNAMIC obstacles along their predicted path.
- **FR-008**: System MUST wire behavior tree nodes to subscribe to live frontier and auction result topics instead of returning hardcoded values.
- **FR-009**: System MUST implement pose graph merging logic that publishes a merged `OccupancyGrid` to `/swarm/global_map` upon rendezvous.
- **FR-010**: System MUST fix the launch file to respect `num_robots` and remove references to non-existent executables.
- **FR-011**: System MUST resolve the `NeighborState` vs `NeighborStateArray` type mismatch so that individual robots aggregate their states correctly for the ORCA filter.
- **FR-012**: System MUST use deterministic random number generation (seeded) for any stochastic behavior.
- **FR-013**: System MUST apply the QoS profiles defined in the DDS topic contract for all publishers and subscribers.

### Key Entities

- **DynamicObstacle**: Tracked entity with pose, velocity, radius, and classification (STATIC=0, SEMI_DYNAMIC=1, DYNAMIC=2), each with a distinct costmap decay rate.
- **AuctionBid**: Contains robot_id, frontier_id, bid_cost computed from the spec formula — deterministic given the same inputs.
- **PoseGraph**: A robot's accumulated keyframes and edges, exchanged peer-to-peer on rendezvous for map merging.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: All robot nodes run for at least 10 minutes under concurrent message delivery without any deadlock or crash.
- **SC-002**: The obstacle tracker publishes `ObstacleArray` messages at ≥ 9 Hz (target 10 Hz, ≤10% tolerance).
- **SC-003**: Given identical inputs, the auction system produces identical task assignments across repeated runs (determinism).
- **SC-004**: Dynamic obstacles are inflated with Gaussian falloff along a 1.5-second predicted trajectory.
- **SC-005**: Behavior tree nodes consume live topic data; no hardcoded return values remain in any BT node.
- **SC-006**: The `num_robots` launch parameter correctly controls the number of spawned robot groups (verified for values 3, 4, and 5).
- **SC-007**: Two robots within 3.0m of each other trigger a map merge and produce a non-empty `/swarm/global_map` message.

## Assumptions

- The existing `swarm_nav_msgs` message definitions are correct and do not need schema changes.
- The project will initially implement a simplified graph merge (local map overlay) rather than full ICP + g2o optimization, which requires the `mrg_slam` external dependency to be integrated later.
- The RVO2 library integration is deferred to a separate follow-up feature; this feature replaces the naive ORCA scaling with a proper geometric velocity-obstacle (VO) computation implemented from scratch in C++ without external dependencies.
- The BehaviorTree.CPP v3→v4 migration is included in this scope to align with the original spec requirement.

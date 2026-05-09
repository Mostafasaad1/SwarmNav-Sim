---
description: "Task list for completing all SwarmNav-Sim core node implementations"
---

# Tasks: Complete All SwarmNav-Sim Core Node Implementations

**Input**: Design documents from `/specs/002-complete-core-nodes/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: No new packages — verify the existing workspace builds and message generation is intact.

- [ ] T001 Verify `swarm_nav_msgs` builds cleanly with `colcon build --packages-select swarm_nav_msgs` in `src/swarm_nav_msgs/`
- [ ] T002 Verify all existing packages compile with `colcon build --symlink-install` from workspace root

**Checkpoint**: Baseline compiles without errors — all modifications happen on a known-good foundation.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Runtime-critical fixes that MUST be complete before ANY user story logic can be validated.

**⚠️ CRITICAL**: All user story work depends on these deadlock fixes being applied first.

- [ ] T003 [P] Remove redundant `std::lock_guard<std::mutex> lock(mutex_)` from `computeOrcaVelocity()` at line ~151 in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`
- [ ] T004 [P] Remove redundant `std::lock_guard<std::mutex> lock(mutex_)` from `calculateBidCost()` at line ~291 in `src/swarm_nav_coordination/src/auctioneer_node.cpp`
- [ ] T005 [P] Replace `rand()` with deterministic `std::mt19937` seeded from `robot_id_` hash in `src/swarm_nav_coordination/src/auctioneer_node.cpp`
- [ ] T006 [P] Rename local struct `Frontier` to `FrontierData` in `src/swarm_nav_coordination/src/frontier_detector_node.cpp` to avoid shadowing `swarm_nav_msgs::msg::Frontier`
- [ ] T007 [P] Rename local struct `NeighborState` to `NeighborData` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp` to avoid shadowing `swarm_nav_msgs::msg::NeighborState`

**Checkpoint**: Foundation ready — all nodes can start and receive messages without deadlocking.

---

## Phase 3: User Story 2 — Obstacle Tracking Pipeline (Priority: P1) 🎯 MVP

**Goal**: Enable the obstacle tracker to broadcast `ObstacleArray` messages so the dynamic costmap layer receives data.

**Independent Test**: Launch `obstacle_tracker_node`, run `ros2 topic echo /swarm/tracked_obstacles --once`, and verify a populated `ObstacleArray` arrives.

### Implementation for User Story 2

- [ ] T008 [US2] Add `#include "swarm_nav_msgs/msg/obstacle_array.hpp"` to `src/swarm_nav_navigation/src/obstacle_tracker_node.cpp`
- [ ] T009 [US2] Uncomment `obstacle_pub_` publisher declaration and initialization for `/swarm/tracked_obstacles` in `src/swarm_nav_navigation/src/obstacle_tracker_node.cpp`
- [ ] T010 [US2] Implement `publishObstacles()` to create and publish `ObstacleArray` from the `obstacles_` map in `src/swarm_nav_navigation/src/obstacle_tracker_node.cpp`

**Checkpoint**: `/swarm/tracked_obstacles` publishes at 10 Hz with 8 test obstacles.

---

## Phase 4: User Story 3 — Spec-Compliant Auction and Frontier Logic (Priority: P1)

**Goal**: Implement the spec-defined utility formula, bid cost formula, and Vickrey tie-breaking rule.

**Independent Test**: Launch three robots, inject five frontiers, verify deterministic winner assignment with correct cost values and tie-breaking by robot_id.

### Implementation for User Story 3

- [ ] T011 [US3] Implement `calculateUtility()` as `size * 0.1 + info_gain * 0.5` where `info_gain` = count of unknown cells within 3m radius of centroid in `src/swarm_nav_coordination/src/frontier_detector_node.cpp`
- [ ] T012 [US3] Store latest map pointer as a member and use it in `calculateUtility()` for information gain lookup in `src/swarm_nav_coordination/src/frontier_detector_node.cpp`
- [ ] T013 [US3] Implement `calculateBidCost()` with formula `distance * 1.0 + travel_time * 0.5 + obstacle_density * 2.0 + task_switch * 0.3` in `src/swarm_nav_coordination/src/auctioneer_node.cpp`
- [ ] T014 [US3] Add secondary sort by `robot_id` (lexicographically lower wins) for equal-cost bids in `resolveAuction()` in `src/swarm_nav_coordination/src/auctioneer_node.cpp`

**Checkpoint**: Auction produces deterministic, spec-compliant results for identical inputs.

---

## Phase 5: User Story 4 — Classification-Aware Dynamic Obstacle Avoidance (Priority: P2)

**Goal**: Differentiate STATIC / SEMI_DYNAMIC / DYNAMIC obstacles with distinct costmap decay rates and Gaussian inflation.

**Independent Test**: Inject three obstacle types via `/swarm/tracked_obstacles`, capture costmap, verify STATIC=permanent, SEMI_DYNAMIC=5s decay, DYNAMIC=2s decay with Gaussian falloff.

### Implementation for User Story 4

- [ ] T015 [US4] Uncomment `#include "swarm_nav_msgs/msg/obstacle_array.hpp"` in `src/swarm_nav_navigation/plugins/dynamic_obstacle_layer.hpp`
- [ ] T016 [US4] Add `rclcpp::Time last_seen` field to `DynamicObstacle` struct and populate it in `obstacleCallback()` in `src/swarm_nav_navigation/plugins/dynamic_obstacle_layer.cpp`
- [ ] T017 [US4] Implement classification-based decay in `updateCosts()`: STATIC=permanent LETHAL, SEMI_DYNAMIC=linear 5s decay, DYNAMIC=exponential 2s decay in `src/swarm_nav_navigation/plugins/dynamic_obstacle_layer.cpp`
- [ ] T018 [US4] Replace linear inflation with Gaussian falloff `C = C_max * exp(-d² / (2*σ²))` where `σ = obstacle_radius + robot_radius + 0.2` in `inflateObstacle()` in `src/swarm_nav_navigation/plugins/dynamic_obstacle_layer.cpp`
- [ ] T019 [US4] Add predictive trajectory inflation for DYNAMIC obstacles along their velocity vector (multiple sample points over 1.5s horizon) in `src/swarm_nav_navigation/plugins/dynamic_obstacle_layer.cpp`

**Checkpoint**: Costmap visually shows distinct inflation patterns for each classification.

---

## Phase 6: User Story 5 — Behavior Tree Integration with Live Data (Priority: P2)

**Goal**: Wire all BT nodes to real ROS 2 topics and migrate from BT.CPP v3 to v4.

**Independent Test**: Run the mission BT for one robot with a mock map; verify BT reads real frontier count and receives real auction assignments.

### Implementation for User Story 5

- [ ] T020 [P] [US5] Change `#include <behaviortree_cpp_v3/...>` to `#include <behaviortree_cpp/...>` in `src/swarm_nav_coordination/src/bt_nodes/map_coverage_check.cpp`
- [ ] T021 [P] [US5] Change `#include <behaviortree_cpp_v3/...>` to `#include <behaviortree_cpp/...>` in `src/swarm_nav_coordination/src/bt_nodes/frontier_detector_bt.cpp`
- [ ] T022 [P] [US5] Change `#include <behaviortree_cpp_v3/...>` to `#include <behaviortree_cpp/...>` in `src/swarm_nav_coordination/src/bt_nodes/run_auction_bt.cpp`
- [ ] T023 [US5] Change `find_package(behaviortree_cpp_v3)` to `find_package(behaviortree_cpp)` and update target references in `src/swarm_nav_coordination/CMakeLists.txt`
- [ ] T024 [US5] Wire `FrontierDetectorBT::tick()` to subscribe to `frontiers` topic and output real frontier count (remove hardcoded `frontier_count = 5`) in `src/swarm_nav_coordination/src/bt_nodes/frontier_detector_bt.cpp`
- [ ] T025 [US5] Wire `RunAuctionBT` to subscribe to `/swarm/auction/result`, trigger real auction participation via `/swarm/auction/announce`, and output real assigned frontier ID (remove hardcoded `"frontier_1"`) in `src/swarm_nav_coordination/src/bt_nodes/run_auction_bt.cpp`

**Checkpoint**: BT nodes consume and produce live data; no hardcoded return values remain.

---

## Phase 7: User Story 6 — Graph Merging and Global Map Publication (Priority: P2)

**Goal**: Implement simplified map overlay merging and publish `/swarm/global_map` on rendezvous.

**Independent Test**: Launch two graph merge nodes, inject neighbor states within 3.0m, verify a non-empty `OccupancyGrid` appears on `/swarm/global_map`.

### Implementation for User Story 6

- [ ] T026 [US6] Add a subscription to neighbor robot's `map` topic (dynamically constructed from `neighbor_id`) in `src/swarm_nav_slam/src/graph_merge_node.cpp`
- [ ] T027 [US6] Implement `mergeGraphs()` as cell-wise max occupancy grid overlay using TF transform from neighbor map frame to own map frame in `src/swarm_nav_slam/src/graph_merge_node.cpp`
- [ ] T028 [US6] Publish the merged grid to `/swarm/global_map` via `global_map_pub_` after successful merge in `src/swarm_nav_slam/src/graph_merge_node.cpp`

**Checkpoint**: `/swarm/global_map` publishes a merged occupancy grid when two robots are within 3.0m.

---

## Phase 8: User Story 1 Enhancement — ORCA Velocity Obstacle Algorithm (Priority: P2)

**Goal**: Replace naive distance scaling with a proper velocity-obstacle (VO) computation for collision-free navigation.

**Independent Test**: Launch ORCA filter with two simulated neighbors on converging paths; verify output velocity avoids the VO cones and does not simply scale to zero.

### Implementation for User Story 1

- [ ] T029 [US1] Implement truncated velocity obstacle (VO) cone computation for each neighbor in `computeOrcaVelocity()` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`
- [ ] T030 [US1] Implement preferred velocity projection onto nearest safe point outside all VO cones in `computeOrcaVelocity()` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`
- [ ] T031 [US1] Add velocity clamping to `max_linear_velocity` and `max_angular_velocity` parameters in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`

**Checkpoint**: ORCA filter produces geometrically correct collision-free velocities.

---

## Phase 9: User Story 7 — Launch File and Integration Correctness (Priority: P3)

**Goal**: Fix launch file to respect `num_robots`, create the neighbor state aggregator, and remove broken executable references.

**Independent Test**: Run `ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3` and verify exactly 3 robots spawn with no missing executable errors.

### Implementation for User Story 7

- [ ] T032 [US7] Create `neighbor_state_aggregator_node.cpp` in `src/swarm_nav_navigation/src/` — subscribes to `/swarm/robot_state`, aggregates into `NeighborStateArray`, publishes to `/swarm/neighbor_states` at 10 Hz
- [ ] T033 [US7] Add `neighbor_state_aggregator_node` executable target to `src/swarm_nav_navigation/CMakeLists.txt`
- [ ] T034 [US7] Change ORCA filter `state_pub_` topic from `/swarm/neighbor_states` to `/swarm/robot_state` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`
- [ ] T035 [US7] Fix `swarm.launch.py` to conditionally spawn robots based on `num_robots` parameter (replace hardcoded `range(5)` with dynamic range) in `src/swarm_nav_bringup/launch/swarm.launch.py`
- [ ] T036 [US7] Remove or replace the `global_map_merger` Node reference (non-existent executable) in `src/swarm_nav_bringup/launch/swarm.launch.py`
- [ ] T037 [US7] Add `neighbor_state_aggregator_node` launch action to `swarm.launch.py` in `src/swarm_nav_bringup/launch/swarm.launch.py`

**Checkpoint**: Full system launches cleanly with configurable robot count and no errors.

---

## Phase 10: Polish & Cross-Cutting Concerns

**Purpose**: QoS alignment, documentation, and final verification.

- [ ] T038 [P] Apply QoS corrections per `contracts/dds-qos-corrections.md`: change `/swarm/auction/announce` to `rclcpp::QoS(1).best_effort()` in `src/swarm_nav_coordination/src/auctioneer_node.cpp`
- [ ] T039 [P] Apply QoS corrections: change `/swarm/neighbor_states` subscriptions to `rclcpp::SensorDataQoS()` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp` and `src/swarm_nav_slam/src/graph_merge_node.cpp`
- [ ] T040 [P] Apply QoS corrections: change `/swarm/robot_state` publisher to `rclcpp::SensorDataQoS()` in `src/swarm_nav_navigation/src/orca_velocity_filter_node.cpp`
- [ ] T041 Update `IMPLEMENTATION_SUMMARY.md` to reflect all completed fixes and remove "Known Limitations" entries that are resolved
- [ ] T042 Update `README.md` with corrected build instructions and smoke test commands from `quickstart.md`
- [ ] T043 Run full workspace `colcon build` and verify zero warnings zero errors

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories
- **User Stories (Phase 3-9)**: All depend on Foundational phase completion
  - US2 (Phase 3) and US3 (Phase 4) can proceed in parallel
  - US4 (Phase 5) depends on US2 (needs obstacle data flowing)
  - US5 (Phase 6) depends on US3 (needs working auction for BT wiring)
  - US6 (Phase 7) can proceed independently after Phase 2
  - US1 enhancement (Phase 8) can proceed independently after Phase 2
  - US7 (Phase 9) depends on US1 enhancement (aggregator feeds ORCA)
- **Polish (Phase 10)**: Depends on all user stories being complete

### User Story Dependencies

- **US2 (P1)**: After Phase 2 — independent
- **US3 (P1)**: After Phase 2 — independent
- **US4 (P2)**: After US2 (needs obstacle data)
- **US5 (P2)**: After US3 (needs working auction)
- **US6 (P2)**: After Phase 2 — independent
- **US1+ (P2)**: After Phase 2 — independent
- **US7 (P3)**: After US1+ (needs aggregator pattern)

### Parallel Opportunities

- All foundational tasks (T003-T007) can run in parallel — different files, no dependencies
- US2 (T008-T010) and US3 (T011-T014) can run in parallel — different packages
- US6 (T026-T028) and US1+ (T029-T031) can run in parallel — different packages
- BT header changes (T020-T022) can run in parallel — different files
- All QoS corrections (T038-T040) can run in parallel — different files

---

## Implementation Strategy

### MVP First (US2 + US3 Only)

1. Complete Phase 1: Setup verification
2. Complete Phase 2: Foundational deadlock fixes (CRITICAL)
3. Complete Phase 3: US2 — Obstacle pipeline live
4. Complete Phase 4: US3 — Auction/frontier spec compliance
5. **STOP and VALIDATE**: Run quickstart smoke tests

### Incremental Delivery

1. Setup + Foundational → Nodes don't crash
2. US2 → Obstacle data flows through the pipeline
3. US3 → Auction produces correct task assignments
4. US4 → Costmap properly handles dynamic obstacles
5. US5 → BT drives real exploration behavior
6. US6 → Map merging produces global map
7. US1+ → ORCA provides collision-free velocities
8. US7 → Full system launches cleanly
9. Polish → QoS correct, docs updated

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Commit after each phase completion
- Stop at any checkpoint to validate story independently

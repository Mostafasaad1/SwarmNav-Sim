---
description: "Task list for SwarmNav-Sim implementation"
---

# Tasks: SwarmNav-Sim: Decentralized Multi-Robot Warehouse Exploration

**Input**: Design documents from `/specs/001-swarm-nav-sim/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [ ] T001 Create ROS 2 workspace structure for `swarm_nav_sim`
- [ ] T002 Initialize `swarm_nav_bringup`, `swarm_nav_slam`, `swarm_nav_navigation`, `swarm_nav_coordination`, `swarm_nav_msgs`, and `swarm_nav_evaluation` packages
- [ ] T003 Create `warehouse_world.launch.py` and URDF for robot models in `swarm_nav_bringup/launch/`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T004 [P] Create `Frontier.msg` and `FrontierArray.msg` in `swarm_nav_msgs/msg/`
- [ ] T005 [P] Create `Obstacle.msg` and `ObstacleArray.msg` in `swarm_nav_msgs/msg/`
- [ ] T006 [P] Create `NeighborState.msg` and `NeighborStateArray.msg` in `swarm_nav_msgs/msg/`
- [ ] T007 [P] Create `AuctionAnnounce.msg`, `AuctionBid.msg`, `AuctionResult.msg` in `swarm_nav_msgs/msg/`
- [ ] T008 Configure ROS 2 Discovery Server rules for inter-robot communication

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Multi-Robot Collaborative SLAM (Priority: P1)

**Goal**: Robots independently map surroundings and merge graphs peer-to-peer upon rendezvous.

**Independent Test**: Launch two robots, verify they build independent maps, and successfully merge their pose graphs over DDS when within 3.0m of each other.

### Implementation for User Story 1

- [ ] T009 [P] [US1] Configure `mrg_slam_multirobot.yaml` in `swarm_nav_bringup/config/`
- [ ] T010 [US1] Create `graph_merge_node.cpp` in `swarm_nav_slam/src/` to handle pose graph exchange and optimization
- [ ] T011 [US1] Integrate `mrg_slam` and graph merge nodes into `swarm.launch.py` for each namespace

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Decentralized Task Allocation (Priority: P1)

**Goal**: Robots autonomously identify frontiers and use a Vickrey auction to distribute tasks.

**Independent Test**: Launch three robots, verify they detect frontiers, announce auctions, and resolve task assignments without duplicates.

### Implementation for User Story 2

- [ ] T012 [P] [US2] Implement `frontier_detector_node.cpp` in `swarm_nav_coordination/src/`
- [ ] T013 [P] [US2] Implement `auctioneer_node.cpp` (Vickrey logic) in `swarm_nav_coordination/src/`
- [ ] T014 [P] [US2] Implement `map_coverage_check.cpp` BT node in `swarm_nav_coordination/src/bt_nodes/`
- [ ] T015 [P] [US2] Implement `frontier_detector_bt.cpp` and `run_auction_bt.cpp` BT nodes
- [ ] T016 [US2] Create `mission_tree.xml` in `swarm_nav_bringup/config/behavior_trees/` using BT.CPP v4
- [ ] T017 [US2] Integrate coordination nodes into `swarm.launch.py`

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - Classical Dynamic Obstacle Avoidance (Priority: P1)

**Goal**: Robots safely navigate using Nav2 TEB and ORCA without machine learning.

**Independent Test**: Launch multiple robots and NPCs crossing paths, verify zero collisions using geometric/optimization-based layers.

### Implementation for User Story 3

- [ ] T018 [P] [US3] Implement `dynamic_obstacle_layer.hpp` and `dynamic_obstacle_layer.cpp` in `swarm_nav_navigation/plugins/`
- [ ] T019 [P] [US3] Implement `obstacle_tracker_node.cpp` in `swarm_nav_navigation/src/` to broadcast NPC tracks
- [ ] T020 [US3] Configure `robot_X_nav2.yaml` and `teb_local_planner.yaml` in `swarm_nav_bringup/config/`
- [ ] T021 [P] [US3] Implement `orca_velocity_filter_node.cpp` using RVO2-ROS2 in `swarm_nav_navigation/src/`
- [ ] T022 [US3] Integrate Nav2, TEB, and ORCA filter nodes into `swarm.launch.py`

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Evaluation metrics, debugging scripts, and final integration checks

- [ ] T023 [P] Implement `coverage_evaluator.py` in `swarm_nav_evaluation/scripts/`
- [ ] T024 [P] Implement `collision_monitor.py` and `slam_metrics.py` in `swarm_nav_evaluation/scripts/`
- [ ] T025 Create `evaluation.launch.py` in `swarm_nav_evaluation/launch/`
- [ ] T026 Execute 10 benchmark test runs and document ATE/collision performance
- [ ] T027 Update `README.md` and `TUNING.md`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel
- **Polish (Final Phase)**: Depends on all user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2)
- **User Story 2 (P1)**: Can start after Foundational (Phase 2)
- **User Story 3 (P1)**: Can start after Foundational (Phase 2)

### Parallel Opportunities

- All custom messages (T004-T007) in Phase 2 can be developed in parallel.
- Navigation layers (T018-T020) and Action BT nodes (T014-T015) within their respective user story phases can be implemented simultaneously.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently via ROS 2 CLI and RViz.

### Incremental Delivery

1. Complete Setup + Foundational → Messages ready
2. Add User Story 1 → Test map merging → SLAM complete
3. Add User Story 2 → Test BT auction mechanics → Coordination complete
4. Add User Story 3 → Test TEB/ORCA layers → Navigation complete
5. Phase 6: Final evaluation execution.

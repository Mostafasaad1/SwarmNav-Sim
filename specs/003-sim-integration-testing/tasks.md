# Tasks: Simulator Integration & Full System Testing

**Input**: Design documents from `specs/003-sim-integration-testing/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, contracts/simulator-bridge.md, quickstart.md

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and dependency tooling

- [ ] T001 Create `setup_dependencies.sh` at project root with shebang, usage text, and `--dry-run` / `--no-sim` flag parsing
- [ ] T002 Add ROS 2 Humble verification check to `setup_dependencies.sh` (check `$ROS_DISTRO == humble`)
- [ ] T003 [P] Add BehaviorTree.CPP v4 install section to `setup_dependencies.sh` (`ros-humble-behaviortree-cpp` with `dpkg -s` idempotency guard)
- [ ] T004 [P] Add Nav2 stack install section to `setup_dependencies.sh` (`ros-humble-navigation2`, `ros-humble-nav2-bringup`, `ros-humble-teb-local-planner`, `ros-humble-nav2-costmap-2d`, `ros-humble-pluginlib`)
- [ ] T005 [P] Add Gazebo Fortress install section to `setup_dependencies.sh` (`ros-humble-ros-gz`, guarded by `--no-sim` flag)
- [ ] T006 Add `rosdep install --from-paths src --ignore-src -r -y` catch-all section to `setup_dependencies.sh`
- [ ] T007 Make `setup_dependencies.sh` executable and verify end-to-end dry-run: `./setup_dependencies.sh --dry-run`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: URDF plugins and world enhancements that ALL simulator-dependent stories need

**⚠️ CRITICAL**: No simulator launch or test can work until this phase is complete

- [ ] T008 Add Gazebo DiffDrive system plugin `<gazebo>` block to `src/swarm_nav_bringup/urdf/swarm_robot.urdf.xacro` — configure `left_wheel_joint`/`right_wheel_joint`, `wheel_separation=0.4`, `wheel_radius=0.05`, topics `cmd_vel`/`odom`
- [ ] T009 Add 2D LiDAR sensor `<gazebo reference="laser_link">` block with `gpu_lidar` type to `src/swarm_nav_bringup/urdf/swarm_robot.urdf.xacro` — 360 samples, min/max angle ±π, range 0.12–12.0m, 10 Hz, topic `scan`
- [ ] T010 [P] Add `JointStatePublisher` system plugin block to `src/swarm_nav_bringup/urdf/swarm_robot.urdf.xacro` for wheel joint states
- [ ] T011 [P] Add caster ball link and fixed joint to `src/swarm_nav_bringup/urdf/swarm_robot.urdf.xacro` for 3-point stability (small sphere at rear of base_link)
- [ ] T012 Add 4 shelving aisle models (10m × 0.3m × 1.5m boxes) to `src/swarm_nav_bringup/worlds/warehouse.world` at Y offsets -10, -3, 3, 10 with 2 cross-aisles for connectivity, ensuring corridors ≥2m wide
- [ ] T013 Add `ros-humble-ros-gz` and `ros-humble-ros-gz-bridge` as `<exec_depend>` in `src/swarm_nav_bringup/package.xml`; add `launch_testing` and `launch_testing_ament_cmake` as `<test_depend>`
- [ ] T014 [P] Add `ament_cmake_gtest` as `<test_depend>` in `src/swarm_nav_coordination/package.xml`, `src/swarm_nav_navigation/package.xml`, and `src/swarm_nav_slam/package.xml`

**Checkpoint**: URDF has simulator plugins, world has obstacles, package dependencies declared. Ready for launch system.

---

## Phase 3: User Story 1 — One-Command Dependency Installation (Priority: P1) 🎯 MVP

**Goal**: Developers can install all optional dependencies with a single script and build the full workspace.

**Independent Test**: Run `./setup_dependencies.sh --dry-run`, then `colcon build`, verify BT library and costmap plugin are built.

### Implementation for User Story 1

- [ ] T015 [US1] Add CoppeliaSim bridge compilation instructions (echo-only) to `setup_dependencies.sh` — print `simROS2` clone/build steps when `--coppeliasim` flag is passed
- [ ] T016 [US1] Verify `colcon build --symlink-install` succeeds with all deps installed — confirm `libswarm_bt_nodes.so` and `libdynamic_obstacle_layer.so` appear in `install/` directory
- [ ] T017 [US1] Verify `colcon build` succeeds WITHOUT optional deps — confirm build completes with only CMake warnings (not errors) about skipped BT nodes and costmap plugin

**Checkpoint**: US1 complete — single script installs everything, build succeeds both with and without optional deps.

---

## Phase 4: User Story 2 — Simulator World and Robot Spawning (Priority: P1)

**Goal**: Launch the full warehouse simulation with N robots in Gazebo Fortress, producing LiDAR and odometry data.

**Independent Test**: `ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3 simulator:=gazebo` — verify 3 robots appear and publish `scan`/`odom` topics.

### Implementation for User Story 2

- [ ] T018 [US2] Create `src/swarm_nav_bringup/config/bridge_config.yaml` defining per-robot topic mappings (`cmd_vel`, `odom`, `scan`) and `/clock` bridge per contracts/simulator-bridge.md
- [ ] T019 [US2] Create `src/swarm_nav_bringup/launch/gazebo.launch.py` — launch Gazebo Fortress with warehouse world using `ros_gz_sim` `GzServer` action, include `ros_gz_bridge` per robot using bridge_config.yaml
- [ ] T020 [US2] Add robot spawn actions to `src/swarm_nav_bringup/launch/gazebo.launch.py` — use `ros_gz_sim::create` node to spawn processed URDF at predefined positions for each robot (loop over `num_robots`)
- [ ] T021 [US2] Add `simulator` launch argument to `src/swarm_nav_bringup/launch/swarm.launch.py` (default: `gazebo`) and use `IncludeLaunchDescription` to delegate to `gazebo.launch.py` (or `coppeliasim.launch.py`)
- [ ] T022 [US2] Add xacro processing node to `src/swarm_nav_bringup/launch/gazebo.launch.py` — run `xacro swarm_robot.urdf.xacro` to produce plain URDF, pass to spawn and `robot_state_publisher`
- [ ] T023 [US2] Verify end-to-end: launch `swarm.launch.py simulator:=gazebo num_robots:=3`, confirm `ros2 topic list` shows `robot_0/scan`, `robot_0/odom`, `robot_0/cmd_vel` for all 3 robots
- [ ] T024 [US2] Verify robot motion: publish `geometry_msgs/msg/Twist` on `/robot_0/cmd_vel`, confirm `/robot_0/odom` position changes

**Checkpoint**: US2 complete — Gazebo Fortress launches with warehouse, 3 robots spawn, LiDAR/odom flow.

---

## Phase 5: User Story 3 — Full Swarm Exploration E2E Run (Priority: P2)

**Goal**: Launch the complete system (sim + SLAM + navigation + coordination) and observe autonomous exploration.

**Independent Test**: Launch full system, run for 5 minutes, verify coverage ≥30%, zero collisions.

### Implementation for User Story 3

- [ ] T025 [US3] Update `src/swarm_nav_bringup/launch/swarm.launch.py` to include Nav2 lifecycle manager and standard Nav2 nodes (planner_server, controller_server, bt_navigator, behavior_server) per robot, parameterized by `config/robot_nav2.yaml`
- [ ] T026 [US3] Add `swarm_bt_nodes` plugin library path to Nav2 `bt_navigator` `plugin_lib_names` in `src/swarm_nav_bringup/config/robot_nav2.yaml`
- [ ] T027 [US3] Wire `NavigateAction` and `WaitForNavigation` in `src/swarm_nav_bringup/config/behavior_trees/mission_tree.xml` — replace `AlwaysSuccess` stubs with `NavigateToPose` action using `{assigned_frontier}` blackboard variable
- [ ] T028 [US3] Create `src/swarm_nav_evaluation/launch/evaluation.launch.py` — include full swarm launch, add `coverage_evaluator` + `collision_monitor` nodes, add configurable timer shutdown (default 300s)
- [ ] T029 [US3] Add `evaluation.launch.py` entry point and new `timer_shutdown_node` script to `src/swarm_nav_evaluation/setup.py`
- [ ] T030 [US3] Create `src/swarm_nav_evaluation/swarm_nav_evaluation/timer_shutdown.py` — a node that calls `rclpy.shutdown()` after configurable duration, triggering evaluation report save
- [ ] T031 [US3] Verify E2E: launch `evaluation.launch.py` with 3 robots for 5 minutes, check `coverage_results.json` shows ≥30% coverage and `collision_results.json` shows 0 collisions

**Checkpoint**: US3 complete — full swarm explores autonomously, metrics are collected.

---

## Phase 6: User Story 4 — Automated Test Suite (Priority: P2)

**Goal**: Automated test suite runnable via `colcon test` covering unit and integration levels.

**Independent Test**: `colcon test && colcon test-result --verbose` — all tests pass.

### Unit Tests

- [ ] T032 [P] [US4] Create `src/swarm_nav_coordination/test/test_auctioneer.cpp` — gtest: verify node constructor, parameter declaration (`robot_id`, `bid_timeout_ms`, `nominal_speed`), bid cost formula returns expected value for known inputs
- [ ] T033 [P] [US4] Create `src/swarm_nav_navigation/test/test_orca_filter.cpp` — gtest: verify node constructor, parameter declaration (6 params including `max_linear_velocity`, `max_angular_velocity`), velocity clamping logic
- [ ] T034 [P] [US4] Create `src/swarm_nav_navigation/test/test_obstacle_tracker.cpp` — gtest: verify node constructor, publisher is created on `/swarm/tracked_obstacles`
- [ ] T035 [P] [US4] Create `src/swarm_nav_slam/test/test_graph_merge.cpp` — gtest: verify node constructor, cell-wise max merge logic on synthetic OccupancyGrid data
- [ ] T036 [P] [US4] Create `src/swarm_nav_evaluation/test/test_coverage_evaluator.py` — pytest: verify coverage calculation on synthetic OccupancyGrid (50% known → 50% coverage)
- [ ] T037 [P] [US4] Create `src/swarm_nav_evaluation/test/test_collision_monitor.py` — pytest: verify collision detection on synthetic Odometry poses (distance < threshold → collision recorded)

### CMake/Build Integration

- [ ] T038 [US4] Add `ament_add_gtest(test_auctioneer ...)` target to `src/swarm_nav_coordination/CMakeLists.txt` inside `BUILD_TESTING` block
- [ ] T039 [P] [US4] Add `ament_add_gtest(test_orca_filter ...)` and `ament_add_gtest(test_obstacle_tracker ...)` targets to `src/swarm_nav_navigation/CMakeLists.txt`
- [ ] T040 [P] [US4] Add `ament_add_gtest(test_graph_merge ...)` target to `src/swarm_nav_slam/CMakeLists.txt`

### Integration Tests

- [ ] T041 [US4] Create `src/swarm_nav_bringup/test/test_system_launch.py` — launch_testing: launch full system with Gazebo for 60s, assert all `scan`/`odom` topics publish ≥1 Hz, assert `/swarm/neighbor_states` publishes, assert no node crashes
- [ ] T042 [US4] Create `src/swarm_nav_bringup/test/test_topic_publishing.py` — launch_testing: launch system, publish `cmd_vel`, assert `odom` position changes within 10s
- [ ] T043 [US4] Add `add_launch_test(test/test_system_launch.py ...)` and `add_launch_test(test/test_topic_publishing.py ...)` to `src/swarm_nav_bringup/CMakeLists.txt` inside `BUILD_TESTING` block
- [ ] T044 [US4] Verify full suite: `colcon test --packages-select swarm_nav_coordination swarm_nav_navigation swarm_nav_slam swarm_nav_evaluation swarm_nav_bringup && colcon test-result --verbose` — all pass

**Checkpoint**: US4 complete — `colcon test` runs all unit + integration tests with pass/fail output.

---

## Phase 7: User Story 5 — CoppeliaSim Backend (Priority: P3)

**Goal**: CoppeliaSim as an alternative simulator backend with identical topic interface.

**Independent Test**: `ros2 launch swarm_nav_bringup swarm.launch.py simulator:=coppeliasim num_robots:=3` — robots spawn, topics match Gazebo backend.

### Implementation for User Story 5

- [ ] T045 [P] [US5] Create `coppeliasim/README.md` with CoppeliaSim installation instructions, `simROS2` compilation steps, and `COPPELIASIM_ROOT_DIR` environment variable setup
- [ ] T046 [US5] Create `coppeliasim/scenes/warehouse.ttt` — CoppeliaSim scene with warehouse walls (40m × 60m), 4 shelving aisles, 3 differential-drive robot models with proximity sensors (LiDAR emulation)
- [ ] T047 [US5] Add `simROS2` Lua scripts to each robot in `warehouse.ttt` — subscribe to `/{robot_id}/cmd_vel` (Twist), publish `/{robot_id}/odom` (Odometry) and `/{robot_id}/scan` (LaserScan)
- [ ] T048 [US5] Create `src/swarm_nav_bringup/launch/coppeliasim.launch.py` — launch CoppeliaSim with scene file, no `ros_gz_bridge` needed (simROS2 publishes directly)
- [ ] T049 [US5] Add error handling to `src/swarm_nav_bringup/launch/swarm.launch.py` — if `simulator` value is unrecognized, raise `LaunchError` listing available backends
- [ ] T050 [US5] Verify CoppeliaSim backend: launch with `simulator:=coppeliasim`, confirm `ros2 topic list` shows same topics as Gazebo backend

**Checkpoint**: US5 complete — CoppeliaSim works as alternative backend with identical ROS 2 topic interface.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Final documentation, cleanup, and validation

- [ ] T051 [P] Update project `README.md` with simulator setup instructions, link to `setup_dependencies.sh`, and quickstart commands
- [ ] T052 [P] Update `specs/003-sim-integration-testing/quickstart.md` with verified commands and expected outputs from actual test runs
- [ ] T053 Run full quickstart.md validation: fresh workspace → `setup_dependencies.sh` → `colcon build` → `ros2 launch` → `colcon test` → confirm all steps succeed
- [ ] T054 Verify CMake optional-dependency warnings: build without BT.CPP, Nav2, and Gazebo — confirm clear warning messages for each skipped feature (FR-011)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion — BLOCKS all user stories
- **US1 (Phase 3)**: Depends on Phase 1 (setup script) — verifies build
- **US2 (Phase 4)**: Depends on Phase 2 (URDF plugins, world, package.xml) — launches sim
- **US3 (Phase 5)**: Depends on US2 (running sim) — adds Nav2 + full exploration
- **US4 (Phase 6)**: Depends on Phase 2 (test deps declared); unit tests can start after Phase 2, integration tests need US2
- **US5 (Phase 7)**: Depends on US2 (launch architecture established) — adds alternative backend
- **Polish (Phase 8)**: Depends on all desired user stories complete

### User Story Dependencies

- **US1 (P1)**: Independent — only needs Phase 1
- **US2 (P1)**: Needs Phase 2 (URDF + world) — independent of US1
- **US3 (P2)**: Needs US2 (running sim) + Nav2 installed (US1)
- **US4 (P2)**: Unit tests independent after Phase 2; integration tests need US2
- **US5 (P3)**: Needs US2 launch architecture; independent of US3/US4

### Parallel Opportunities

- T003, T004, T005 can run in parallel (independent install sections)
- T010, T011 can run in parallel (independent URDF additions)
- T013, T014 can run in parallel (different package.xml files)
- T032–T037 can ALL run in parallel (independent test files)
- T038, T039, T040 can run in parallel (different CMakeLists)
- T045 can run in parallel with T046–T048

---

## Parallel Example: User Story 4

```bash
# Launch all unit test files together (all [P]):
Task: "Create test_auctioneer.cpp in src/swarm_nav_coordination/test/"
Task: "Create test_orca_filter.cpp in src/swarm_nav_navigation/test/"
Task: "Create test_obstacle_tracker.cpp in src/swarm_nav_navigation/test/"
Task: "Create test_graph_merge.cpp in src/swarm_nav_slam/test/"
Task: "Create test_coverage_evaluator.py in src/swarm_nav_evaluation/test/"
Task: "Create test_collision_monitor.py in src/swarm_nav_evaluation/test/"

# Then wire CMake targets (all [P]):
Task: "Add gtest target to swarm_nav_coordination/CMakeLists.txt"
Task: "Add gtest targets to swarm_nav_navigation/CMakeLists.txt"
Task: "Add gtest target to swarm_nav_slam/CMakeLists.txt"
```

---

## Implementation Strategy

### MVP First (User Story 1 + 2)

1. Complete Phase 1: Setup script
2. Complete Phase 2: URDF plugins + world
3. Complete Phase 3: US1 — verify build
4. Complete Phase 4: US2 — verify sim launch
5. **STOP and VALIDATE**: Robots spawn in Gazebo and publish data

### Incremental Delivery

1. Setup + Foundational → Workspace ready
2. Add US1 → Dependencies installable → Build succeeds (MVP!)
3. Add US2 → Sim launches → Robots publish data
4. Add US3 → Full autonomous exploration → Metrics collected
5. Add US4 → Automated tests → CI-ready
6. Add US5 → CoppeliaSim alternative → Multi-backend support

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Total: **54 tasks** across 8 phases
- Phases A–D (plan.md) map to Phases 1–4 (tasks), E–F to Phase 6, G to Phase 5, H to Phase 7
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently

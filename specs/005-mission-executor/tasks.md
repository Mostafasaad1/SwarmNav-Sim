# Tasks: Mission Executor Node

**Input**: Design documents from `specs/005-mission-executor/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/ROS2_API.md

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1)
- Include exact file paths in descriptions

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and build configuration

- [X] T001 Update `src/swarm_nav_coordination/CMakeLists.txt` to add executable for `mission_executor_node`
- [X] T002 Update `src/swarm_nav_coordination/CMakeLists.txt` to add gtest for `test_mission_executor.cpp`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

- [X] T003 Ensure all custom BT plugins are correctly exported or accessible within `swarm_nav_coordination` for static linking.

**Checkpoint**: Foundation ready - user story implementation can now begin

---

## Phase 3: User Story 1 - Autonomous Exploration (Priority: P1) 🎯 MVP

**Goal**: Execute the mission behavior tree autonomously without manual intervention.

**Independent Test**: Can be fully tested by launching the swarm with the mission executor enabled, and observing the robots automatically begin navigating to explore the map.

### Tests for User Story 1 (MANDATORY per constitution) ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [X] T004 [P] [US1] Create unit test for the executor node lifecycle transitions in `src/swarm_nav_coordination/test/test_mission_executor.cpp`

### Implementation for User Story 1

- [X] T005 [P] [US1] Implement `MissionExecutorNode` class in `src/swarm_nav_coordination/src/mission_executor_node.cpp` with lifecycle manager integration and 10Hz tick timer.
- [X] T006 [US1] Update `src/swarm_nav_bringup/launch/swarm.launch.py` to start the `mission_executor_node` for each robot and configure it to transition to active.

**Checkpoint**: At this point, Autonomous Exploration should be fully functional and testable independently

---

## Phase N: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [X] T007 Run `colcon test` for `swarm_nav_coordination` to verify all tests pass.
- [X] T008 Code cleanup and verify linting (pep257, flake8, uncrustify).
- [ ] T009 Run quickstart.md validation by launching the full simulation.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories.

### Within Each User Story

- Tests MUST be written and FAIL before implementation (Test-First principle).
- Core implementation before integration.

### Parallel Opportunities

- CMake updates in Phase 1 can be done in parallel.
- Test creation (T004) and Node structure creation (T005) can be developed in parallel as long as T004 fails initially.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently using quickstart
5. Demo the swarm exploring the environment autonomously.

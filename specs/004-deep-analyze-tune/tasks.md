# Tasks: Deep Analysis & Tuning Suite

**Input**: Design documents from `/specs/004-deep-analyze-tune/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Tests are included based on the requirement to test the evaluation nodes (e.g. `launch_testing`).

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic dependencies

- [X] T001 Update package dependencies (add `optuna`, `psutil`, `pandas`) in `src/swarm_nav_evaluation/package.xml`
- [X] T002 Update `src/swarm_nav_evaluation/setup.py` and `CMakeLists.txt` for new nodes and launch files

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T003 Implement `system_metrics.py` (psutil collector) in `src/swarm_nav_evaluation/swarm_nav_evaluation/system_metrics.py`
- [X] T004 Create CSV/JSON report writing utilities in `src/swarm_nav_evaluation/swarm_nav_evaluation/report_utils.py`
- [X] T005 Define baseline headless launch configurations in `src/swarm_nav_evaluation/launch/benchmark.launch.py`

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Automated Benchmarking Suite (Priority: P1) 🎯 MVP

**Goal**: Run a suite of automated benchmark scenarios to measure the swarm's performance so that I can establish a reliable baseline.

**Independent Test**: Execute `benchmark_runner` with a sample `scenarios.yaml` and verify a complete report is generated without GUI rendering.

### Implementation for User Story 1

- [X] T006 [P] [US1] Create baseline configurations in `src/swarm_nav_evaluation/config/scenarios.yaml`
- [X] T007 [US1] Implement `BenchmarkScenario` parsing and execution loop in `src/swarm_nav_evaluation/swarm_nav_evaluation/benchmark_runner.py`
- [X] T008 [US1] Integrate `system_metrics.py` data into `PerformanceReport` inside `benchmark_runner.py`
- [X] T009 [US1] Add fail-fast / abort logic on simulation crash/timeout in `benchmark_runner.py`
- [X] T010 [US1] Write test script for `benchmark_runner` in `src/swarm_nav_evaluation/test/test_benchmark_runner.py`

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently. You can run automated sequences of evaluations.

---

## Phase 4: User Story 2 - Parameter Sensitivity Analysis (Priority: P2)

**Goal**: Study the effect of varying key robot parameters (like speed, sensor range) on exploration metrics.

**Independent Test**: Provide a parameter sweep list and verify that the system runs all permutations and outputs a correlation graph/table.

### Implementation for User Story 2

- [X] T011 [US2] Extend `benchmark_runner.py` to parse parameter sweep lists from config and generate permutation sequences.
- [X] T012 [US2] Implement parameter overriding in `launch/benchmark.launch.py` via command line arguments.
- [X] T013 [US2] Add correlation plotting (using pandas) to `report_utils.py` upon sweep completion.

**Checkpoint**: User Stories 1 AND 2 should both work independently. Sensitivity sweeps can now be performed.

---

## Phase 5: User Story 3 - Automated Deep Tuning (Priority: P3)

**Goal**: Automatically tune system parameters using Bayesian Optimization to find the most efficient configuration.

**Independent Test**: Launch the tuner with an objective function (e.g. minimize exploration time) and verify it completes Optuna trials and reports the best parameters.

### Implementation for User Story 3

- [X] T014 [P] [US3] Define tuning bounds and targets in `src/swarm_nav_evaluation/config/tune_space.yaml`
- [X] T015 [US3] Implement Bayesian Optimization loop using `optuna` in `src/swarm_nav_evaluation/swarm_nav_evaluation/bayesian_tuner.py`
- [X] T016 [US3] Create launch wrapper `src/swarm_nav_evaluation/launch/tuning.launch.py` to invoke the tuner node.
- [X] T017 [US3] Write test script for `bayesian_tuner` in `src/swarm_nav_evaluation/test/test_bayesian_tuner.py`

**Checkpoint**: All user stories should now be independently functional.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [X] T018 [P] Documentation updates in `README.md`
- [X] T019 Run quickstart.md validation locally to ensure commands work as expected.
- [X] T020 Code cleanup and flake8/pep257 formatting.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories proceed sequentially in priority order (US1 → US2 → US3) since US2 extends US1's runner.
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2)
- **User Story 2 (P2)**: Extends US1 (Benchmark Runner). Must start after US1.
- **User Story 3 (P3)**: Depends on US1's ability to launch headlessly and retrieve reports.

### Parallel Opportunities

- Configuration file creation (`T006`, `T014`) can be done in parallel with code development.
- Test files (`T010`, `T017`) can be developed in parallel with the core nodes.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently by running a multi-scenario benchmark.

### Incremental Delivery

1. Foundation ready.
2. Add US1 → Test independently → MVP!
3. Add US2 → Test parameter sweeps.
4. Add US3 → Test Optuna bayesian tuning.

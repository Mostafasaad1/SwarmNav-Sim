# Feature Specification: Simulator Integration & Full System Testing

**Feature Branch**: `003-sim-integration-testing`  
**Created**: 2026-05-09  
**Status**: Draft  
**Input**: User description: "Optional dependency installation (BehaviorTree.CPP v4, Nav2), Simulator integration (Gazebo, ign gazebo, coppeliasim — prefer), Full system testing"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - One-Command Dependency Installation (Priority: P1)

As a developer, I want a single setup script that detects and installs all optional dependencies (BehaviorTree.CPP v4, Nav2 stack, simulator backends) so that I can go from a clean Ubuntu 22.04 machine to a fully buildable workspace without hunting for packages manually.

**Why this priority**: Nothing else can be built or tested until the dependency chain is resolved. Currently, `behaviortree_cpp`, `nav2_*`, and simulator packages are optional finds that silently skip features when missing. Developers need a clear, automated path to install them all.

**Independent Test**: Run the setup script on a fresh ROS 2 Humble installation, then run `colcon build` and confirm zero warnings about missing optional dependencies.

**Acceptance Scenarios**:

1. **Given** a clean Ubuntu 22.04 system with ROS 2 Humble base installed, **When** the developer runs the dependency setup script, **Then** BehaviorTree.CPP v4, all required Nav2 packages, and the selected simulator backend are installed and discoverable by CMake.
2. **Given** the setup script has run, **When** the workspace is built with `colcon build`, **Then** the BT node shared library (`swarm_bt_nodes`) is compiled (not skipped) and the dynamic obstacle layer costmap plugin is compiled (not skipped).
3. **Given** a system that already has all dependencies installed, **When** the setup script is re-run, **Then** it detects existing installations, skips redundant work, and completes without errors (idempotent).

---

### User Story 2 - Simulator World and Robot Spawning (Priority: P1)

As a system operator, I want to launch the full warehouse simulation with multiple robots in a physics-enabled simulator so that I can observe and validate multi-robot exploration behavior in a realistic 3D environment.

**Why this priority**: The simulation world is the runtime environment for all robots. Without a working sim, there is no way to generate sensor data (LiDAR scans, odometry) that drive the SLAM and navigation pipelines.

**Independent Test**: Run the simulator launch command with `num_robots:=3`, verify that 3 robot models appear in the simulator viewport and that each robot publishes `scan` and `odom` topics.

**Acceptance Scenarios**:

1. **Given** the simulator and workspace are installed, **When** the operator runs the simulation launch file, **Then** the warehouse world (40m × 60m with shelving aisles) loads and renders in the simulator window.
2. **Given** the world is loaded, **When** `num_robots:=3` is specified, **Then** exactly 3 differential-drive robots are spawned at their defined start positions and each publishes LiDAR data on `robot_X/scan` and odometry on `robot_X/odom`.
3. **Given** spawned robots, **When** a velocity command is published on `robot_X/cmd_vel`, **Then** the corresponding robot moves in the simulator with physics-correct behavior (inertia, collision).
4. **Given** the 3D warehouse environment, **When** a robot's LiDAR sensor fires, **Then** the scan correctly detects shelving obstacles, walls, and other robots within the sensor's 12m range.

---

### User Story 3 - Full Swarm Exploration End-to-End Run (Priority: P2)

As a system operator, I want to launch the complete SwarmNav-Sim system (SLAM + navigation + coordination + simulator) and observe multiple robots autonomously exploring the warehouse, sharing maps, and avoiding collisions so that the entire architecture is validated as a working system.

**Why this priority**: This is the ultimate integration test. Individual nodes have been validated in isolation — this story confirms they work together as a swarm.

**Independent Test**: Launch the full system with 3 robots, let it run for 5 minutes, and verify: (a) global map coverage increases over time, (b) no robot collisions occur, (c) auction results are logged.

**Acceptance Scenarios**:

1. **Given** the full system is launched, **When** robots begin exploring, **Then** each robot's frontier detector identifies unexplored regions and the auctioneer assigns exploration tasks via Vickrey auction.
2. **Given** robots are exploring with ORCA velocity filtering active, **When** two robots approach each other, **Then** the velocity obstacles prevent collision and both robots continue navigating without stopping indefinitely.
3. **Given** two robots come within 3m of each other, **When** the graph merge node detects rendezvous, **Then** a merged global map is published on `/swarm/global_map` that combines both robots' explored areas.
4. **Given** the system has run for 5 minutes, **When** the evaluation node reports metrics, **Then** global map coverage is ≥30% of the warehouse area.

---

### User Story 4 - Automated System Test Suite (Priority: P2)

As a developer, I want an automated test suite that validates core system behaviors without manual observation so that I can run regression tests in CI or before each merge.

**Why this priority**: Manual validation does not scale. An automated test suite catches regressions early and provides confidence that changes in one package don't break another.

**Independent Test**: Run `colcon test` and verify all test cases pass with clear pass/fail output.

**Acceptance Scenarios**:

1. **Given** the workspace is built, **When** `colcon test` is run, **Then** all unit-level node tests execute and report pass/fail.
2. **Given** a running simulation, **When** the integration test launches 3 robots for 60 seconds, **Then** the test asserts: obstacle tracker publishes at ≥9 Hz, ORCA filter produces non-zero velocities, at least one auction result is published.
3. **Given** a regression is introduced (e.g., a deadlock restored), **When** the test suite runs, **Then** the affected test times out and is marked FAIL within 30 seconds.

---

### User Story 5 - CoppeliaSim as Preferred Simulator Backend (Priority: P3)

As a developer, I want to use CoppeliaSim as the primary simulation backend while retaining Gazebo/Ignition as a fallback so that I can leverage CoppeliaSim's faster physics and Python scripting for advanced scenarios.

**Why this priority**: CoppeliaSim is the user's preferred simulator but has a smaller ROS 2 ecosystem. Gazebo/Ignition is the industry standard and easier to set up. Supporting both gives flexibility.

**Independent Test**: Launch the same `swarm.launch.py` with `simulator:=coppeliasim` and verify robots appear and publish sensor data identically to the Gazebo backend.

**Acceptance Scenarios**:

1. **Given** CoppeliaSim is installed with its ROS 2 interface plugin, **When** the operator launches with `simulator:=coppeliasim`, **Then** the warehouse world loads in CoppeliaSim and robots spawn with LiDAR and differential-drive.
2. **Given** the CoppeliaSim backend is running, **When** the same `cmd_vel` commands are issued, **Then** robot behavior (movement, collisions) matches the Gazebo backend within reasonable tolerance.
3. **Given** neither CoppeliaSim nor Gazebo is installed, **When** the user tries to launch the simulator, **Then** a clear error message is displayed listing which simulators are available and how to install them.

---

### Edge Cases

- What happens if Nav2 packages are not installed but the user launches the full system? The launch file should start all non-Nav2 nodes and log a warning listing the missing packages.
- What happens if the simulator process crashes mid-run? Robot nodes should detect the loss of sensor data (topic timeout) and enter a safe-stop state rather than continuing blind.
- What happens if `colcon build` is run without the setup script? Packages with optional dependencies (BT nodes, costmap plugin) should gracefully skip with CMake warnings, not fail.
- What happens if two simulator backends are installed? The `simulator` launch argument selects which one to use; the other is ignored.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The project MUST provide a setup script that installs BehaviorTree.CPP v4 from the ROS 2 Humble repositories or from source.
- **FR-002**: The project MUST provide a setup script that installs the Nav2 stack (`nav2_bringup`, `nav2_bt_navigator`, `nav2_costmap_2d`, `nav2_core`, `navigation2`) from the ROS 2 Humble repositories.
- **FR-003**: The project MUST provide a setup script that installs at least one physics simulator backend (Gazebo Classic, Ignition Gazebo, or CoppeliaSim).
- **FR-004**: The setup script MUST be idempotent — running it multiple times on the same system produces the same result without errors.
- **FR-005**: The project MUST provide a warehouse world file compatible with the selected simulator containing a 40m × 60m floor plan with shelving aisles and open corridors.
- **FR-006**: The project MUST provide a robot model (URDF/SDF) with differential drive, 2D LiDAR (360° scan, 12m range), and odometry publishing.
- **FR-007**: The launch system MUST spawn the user-specified number of robots (1–5) at predefined positions in the simulator world.
- **FR-008**: The launch system MUST support a `simulator` argument to select between available backends (e.g., `gazebo`, `ignition`, `coppeliasim`).
- **FR-009**: The project MUST include integration tests that verify multi-robot communication (topics publishing), obstacle avoidance (no collisions), and map sharing (global map published).
- **FR-010**: The project MUST include unit-level launch tests for each package that verify nodes start without crashing.
- **FR-011**: All CMake optional-dependency guards (`if(behaviortree_cpp_FOUND)`, `if(BUILD_NAV2_PLUGIN)`) MUST produce clear diagnostic messages when dependencies are missing.
- **FR-012**: The evaluation package MUST report coverage percentage, collision count, and exploration time for end-to-end test runs.

### Key Entities

- **Simulator Backend**: Abstraction representing the physics simulator (Gazebo, Ignition, CoppeliaSim), each with its own world file format, spawn API, and sensor plugin set.
- **Robot Model**: A differential-drive mobile robot with a 2D LiDAR sensor, defined in URDF/xacro, instantiated N times with unique namespaces.
- **Warehouse World**: A 40m × 60m indoor environment with static shelving aisles, walls, and open corridors — the arena for exploration.
- **Test Case**: An automated check (unit or integration) that asserts a specific system behavior and produces a pass/fail verdict.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A new developer can go from a fresh ROS 2 Humble installation to a fully built workspace (all optional features compiled) in under 15 minutes using the setup script.
- **SC-002**: The full system (3 robots + simulator + all nodes) starts without errors and runs stably for at least 10 minutes without any node crashes.
- **SC-003**: After 5 minutes of autonomous exploration, global map coverage reaches ≥30% of the warehouse floor area.
- **SC-004**: Zero inter-robot collisions occur during a 10-minute exploration run with 3 robots.
- **SC-005**: The automated test suite completes within 5 minutes and reports ≥90% pass rate on the first run.
- **SC-006**: The system supports at least two simulator backends (Gazebo/Ignition as primary + CoppeliaSim as secondary).
- **SC-007**: `colcon build` succeeds on a system missing optional dependencies, with only warnings (not errors) about skipped features.

## Assumptions

- The target platform is Ubuntu 22.04 with ROS 2 Humble installed from Debian packages (not source-built).
- The setup script uses `apt` and/or `rosdep` for dependency installation; it does not require `sudo` access for workspace-local (source-built) dependencies.
- CoppeliaSim integration uses the `simROS2` interface plugin; the CoppeliaSim application itself is installed externally by the user (the setup script only installs the ROS 2 bridge).
- The existing `warehouse.world` file will be adapted for the selected simulator backend rather than creating separate world files from scratch.
- The evaluation package (`swarm_nav_evaluation`) is Python-based and will be extended with new metrics scripts rather than rewritten.
- Nav2 is used for path planning only — the existing TEB local planner configuration in `robot_nav2.yaml` is assumed to be correct and will not be redesigned in this feature.
- The BT node shared library (`swarm_bt_nodes`) is loaded by Nav2's `bt_navigator` — this feature ensures it builds, but full BT mission orchestration is not in scope (only launch and load testing).

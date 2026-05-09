# Research: Simulator Integration & Full System Testing

**Date**: 2026-05-09
**Feature**: `003-sim-integration-testing`

---

## Decision 1: Primary Simulator Backend

**Decision**: Use **Gazebo Fortress** (Modern Gazebo, the Ignition successor) as primary backend via `ros_gz`.

**Rationale**:
- Gazebo Fortress is the officially paired simulator for ROS 2 Humble.
- Gazebo Classic (Gazebo 11) is end-of-life — no new development, deprecated for new ROS 2 projects.
- Modern Gazebo uses `ros_gz_bridge` for topic bridging rather than monolithic plugins.
- The existing `warehouse.world` is SDF 1.6, which is natively compatible with Gazebo Fortress.
- Multi-robot namespacing is cleaner: `/model/<robot_name>/...` structure.

**Alternatives considered**:
- **Gazebo Classic (11)**: EOL, limited Humble support via `gazebo_ros_pkgs`. Rejected.
- **CoppeliaSim as primary**: Requires external installation + `simROS2` bridge compilation. Higher friction for new contributors. Will be supported as a secondary backend (US5/P3).
- **Isaac Sim**: Mentioned in existing world file comments but requires NVIDIA hardware and proprietary license. Out of scope.

**Key technical notes**:
- Package suite: `ros-humble-ros-gz` (meta-package includes `ros_gz_sim`, `ros_gz_bridge`, `ros_gz_image`)
- Differential drive: `gz-sim-diff-drive-plugin` (not `gazebo_ros_diff_drive`)
- LiDAR: `gz-sim-sensors-system` + `gpu_lidar` or `lidar` sensor type
- The URDF xacro needs `<gazebo>` extension tags referencing Fortress plugin names

---

## Decision 2: BehaviorTree.CPP v4 Installation

**Decision**: Install via apt: `sudo apt install ros-humble-behaviortree-cpp`

**Rationale**:
- Pre-built Debian package available in the ROS 2 Humble repository.
- No source compilation needed.
- CMake `find_package(behaviortree_cpp)` immediately succeeds after install.
- The v4 package name is `behaviortree_cpp` (without version suffix).

**Alternatives considered**:
- **Source build from GitHub**: More complex, only needed for unreleased features. Unnecessary.
- **v3 (`behaviortree_cpp_v3`)**: Legacy, already migrated away in spec-002. Rejected.

---

## Decision 3: Nav2 Installation

**Decision**: Install full Nav2 stack via apt: `sudo apt install ros-humble-navigation2 ros-humble-nav2-bringup ros-humble-teb-local-planner`

**Rationale**:
- Existing `robot_nav2.yaml` references `nav2_teb_controller::TebController` which requires the TEB package.
- The Nav2 costmap layer plugin (`DynamicObstacleLayer`) requires `nav2_costmap_2d` headers at build time.
- Debian packages are well-tested with Humble.

**Key packages needed**:
- `ros-humble-navigation2` (meta-package)
- `ros-humble-nav2-bringup` (launch files, default BTs)
- `ros-humble-teb-local-planner` (TEB controller)
- `ros-humble-nav2-costmap-2d` (for plugin build)
- `ros-humble-pluginlib` (for costmap layer registration)

---

## Decision 4: URDF Gazebo Plugin Strategy

**Decision**: Add Gazebo Fortress plugin tags to existing xacro via conditional blocks.

**Rationale**:
- The URDF currently defines geometry (cylinder base, wheels, LiDAR mount) but has no `<gazebo>` tags for simulator plugins.
- Adding `<gazebo>` blocks with Fortress-compatible plugins keeps the URDF as the single source of truth.
- A Gazebo-specific SDF wrapper can spawn the URDF model with `ros_gz_sim::create`.

**Plugins needed**:
- `gz::sim::systems::DiffDrive` — controls left/right wheels via `cmd_vel`
- `gz::sim::systems::Sensors` — enables sensor simulation
- `gz::sim::systems::Imu` — (optional) for IMU data
- LiDAR sensor: `<sensor type="gpu_lidar">` with 360° scan, 12m range, publishing to `scan` topic

---

## Decision 5: CoppeliaSim Secondary Backend

**Decision**: Provide a separate `.ttt` scene file and a thin launch adapter. The CoppeliaSim application is installed externally; only the ROS 2 bridge (`simROS2`) is workspace-managed.

**Rationale**:
- CoppeliaSim uses its own scene format (`.ttt`), not SDF.
- The `simROS2` plugin is compiled from `https://github.com/CoppeliaRobotics/simROS2` and requires `COPPELIASIM_ROOT_DIR`.
- This is a P3 feature — provide the abstraction but don't block P1/P2 on it.

**Integration pattern**:
- Launch argument `simulator:=coppeliasim` selects an alternative launch include.
- CoppeliaSim launch starts the simulator headless/GUI, loads the `.ttt` scene, and relies on `simROS2` to bridge topics.
- Topic names must match the Gazebo backend (`robot_X/scan`, `robot_X/odom`, `robot_X/cmd_vel`).

---

## Decision 6: Testing Framework

**Decision**: Use `launch_testing` for integration tests, `pytest` for evaluation Python scripts, and `ament_cmake_gtest` for C++ unit tests.

**Rationale**:
- `launch_testing` is the ROS 2 standard for multi-node integration testing. It can launch nodes, wait for topics, and assert conditions.
- The evaluation package is already Python-based (`ament_python`), so pytest is the natural fit.
- C++ node unit tests use `gtest` via `ament_cmake_gtest`.

**Test categories**:
1. **Unit tests** (per-package): Node constructor doesn't crash, parameters are declared correctly, message serialization works.
2. **Integration tests** (bringup package): Launch 3 robots + sim, verify topics publish, verify no crashes after 60s.
3. **E2E evaluation** (evaluation package): Run full exploration, assert coverage ≥30% at 5min, assert 0 collisions.

---

## Decision 7: Setup Script Architecture

**Decision**: Single `setup_dependencies.sh` bash script in the project root with modular sections and `--dry-run` support.

**Rationale**:
- Developers expect a single entry point.
- Modular sections (apt, rosdep, pip) allow partial execution.
- `--dry-run` mode prints what would be installed without executing.
- Idempotent: uses `dpkg -s` checks before `apt install`.

**Sections**:
1. ROS 2 base verification
2. BehaviorTree.CPP v4 (apt)
3. Nav2 stack (apt)
4. Gazebo Fortress + ros_gz (apt)
5. rosdep install (catch-all)
6. (Optional) CoppeliaSim bridge compilation instructions

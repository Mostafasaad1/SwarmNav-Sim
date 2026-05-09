# Data Model: Simulator Integration & Full System Testing

**Feature**: `003-sim-integration-testing`
**Date**: 2026-05-09

---

## Entity: Simulator Backend

Abstraction representing a physics simulator runtime.

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Identifier: `gazebo`, `coppeliasim` |
| `launch_package` | string | ROS 2 package containing launch files |
| `world_file` | path | Path to world/scene file (`.sdf` or `.ttt`) |
| `spawn_method` | enum | `ros_gz_sim::create` or `coppeliasim_scene` |
| `bridge_required` | bool | Whether a topic bridge is needed |

**Relationships**: Each Backend contains 1 World and spawns N Robot Models.

---

## Entity: Robot Model

A differential-drive mobile robot instance in the simulator.

| Field | Type | Description |
|-------|------|-------------|
| `robot_id` | string | Unique namespace: `robot_0`, `robot_1`, ... |
| `namespace` | string | ROS 2 namespace for all topics |
| `spawn_position` | (x, y, yaw) | Initial world-frame position |
| `urdf_path` | path | Path to processed URDF/xacro |
| `lidar_topic` | string | Namespaced scan topic: `/{ns}/scan` |
| `odom_topic` | string | Namespaced odometry: `/{ns}/odom` |
| `cmd_vel_topic` | string | Namespaced velocity command: `/{ns}/cmd_vel` |

**Validation**: `robot_id` must match `robot_N` where N ∈ [0, 4]. Spawn positions must be within world bounds (±20m x, ±30m y).

---

## Entity: Test Case

An automated verification of system behavior.

| Field | Type | Description |
|-------|------|-------------|
| `test_id` | string | e.g., `integration_topic_publish` |
| `type` | enum | `unit`, `integration`, `e2e` |
| `package` | string | Package containing the test |
| `timeout` | seconds | Max execution time before FAIL |
| `assertions` | list | Conditions that must hold |
| `requires_sim` | bool | Whether a running simulator is needed |

---

## Entity: Dependency

An optional software package required for full functionality.

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Human-readable name |
| `apt_package` | string | Debian package name |
| `check_command` | string | Shell command to verify installation |
| `required_by` | list[string] | Packages that need this dependency |
| `optional` | bool | Whether build succeeds without it |

### Instances

| Name | apt_package | check_command | required_by | optional |
|------|-------------|---------------|-------------|----------|
| BehaviorTree.CPP v4 | `ros-humble-behaviortree-cpp` | `dpkg -s ros-humble-behaviortree-cpp` | `swarm_nav_coordination` | yes |
| Nav2 Core | `ros-humble-navigation2` | `dpkg -s ros-humble-navigation2` | `swarm_nav_navigation` | yes |
| Nav2 Costmap 2D | `ros-humble-nav2-costmap-2d` | `dpkg -s ros-humble-nav2-costmap-2d` | `swarm_nav_navigation` | yes |
| TEB Controller | `ros-humble-teb-local-planner` | `dpkg -s ros-humble-teb-local-planner` | `swarm_nav_bringup` | yes |
| Gazebo Fortress | `ros-humble-ros-gz` | `dpkg -s ros-humble-ros-gz` | `swarm_nav_bringup` | yes |
| Pluginlib | `ros-humble-pluginlib` | `dpkg -s ros-humble-pluginlib` | `swarm_nav_navigation` | yes |

---

## State Transitions

### Simulation Lifecycle

```
UNINSTALLED → [setup_dependencies.sh] → INSTALLED
INSTALLED → [colcon build] → BUILT
BUILT → [ros2 launch swarm.launch.py] → RUNNING
RUNNING → [timeout/manual stop] → EVALUATION
EVALUATION → [colcon test / coverage_evaluator] → REPORTED
```

### Test Execution States

```
PENDING → [colcon test] → RUNNING → PASS | FAIL | TIMEOUT
```

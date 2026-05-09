# Contract: Simulator Topic Bridge

**Feature**: `003-sim-integration-testing`

This contract defines the topic interface between the simulator (Gazebo Fortress via `ros_gz_bridge`) and the SwarmNav-Sim ROS 2 nodes.

---

## Per-Robot Topics (namespaced under `/{robot_id}/`)

| Simulator Topic (gz-transport) | ROS 2 Topic | Message Type | Direction | QoS | Bridge Config |
|-------------------------------|-------------|--------------|-----------|-----|---------------|
| `/model/{robot_id}/cmd_vel` | `/{robot_id}/cmd_vel` | `geometry_msgs/msg/Twist` | ROS→Gz | Reliable, depth=10 | `@gz_msgs.Twist[ignition.msgs.Twist` |
| `/model/{robot_id}/odometry` | `/{robot_id}/odom` | `nav_msgs/msg/Odometry` | Gz→ROS | Reliable, depth=10 | `@gz_msgs.Odometry[ignition.msgs.Odometry` |
| `/model/{robot_id}/scan` | `/{robot_id}/scan` | `sensor_msgs/msg/LaserScan` | Gz→ROS | SensorDataQoS | `@gz_msgs.LaserScan[ignition.msgs.LaserScan` |
| `/model/{robot_id}/tf` | `/tf` | `tf2_msgs/msg/TFMessage` | Gz→ROS | Reliable, depth=100 | auto via `gz-sim-pose-publisher-system` |

## Global Topics (no namespace)

| Simulator Topic | ROS 2 Topic | Message Type | Direction | Notes |
|----------------|-------------|--------------|-----------|-------|
| `/clock` | `/clock` | `rosgraph_msgs/msg/Clock` | Gz→ROS | Required for `use_sim_time: true` |

---

## Robot Spawn Contract

Each robot is spawned by calling `ros_gz_sim::create` with:

```yaml
name: "{robot_id}"
file: "swarm_robot.urdf"  # processed URDF (xacro output)
pose:
  x: {spawn_x}
  y: {spawn_y}
  z: 0.05
  roll: 0
  pitch: 0
  yaw: {spawn_yaw}
```

---

## Gazebo Plugin Requirements (in URDF `<gazebo>` tags)

| Plugin | System Name | Attached To | Key Parameters |
|--------|-------------|-------------|----------------|
| Diff Drive | `gz::sim::systems::DiffDrive` | `<model>` | `left_joint=left_wheel_joint`, `right_joint=right_wheel_joint`, `wheel_separation=0.4`, `wheel_radius=0.05`, `max_linear_velocity=0.5` |
| Joint State Publisher | `gz::sim::systems::JointStatePublisher` | `<model>` | publishes joint states for `robot_state_publisher` |
| 2D LiDAR | `<sensor type="gpu_lidar">` | `laser_link` | `samples=360`, `min_angle=-3.14159`, `max_angle=3.14159`, `range_min=0.12`, `range_max=12.0`, `update_rate=10` |

---

## CoppeliaSim Equivalent Contract

When `simulator:=coppeliasim`, the same ROS 2 topics must be published/subscribed by the CoppeliaSim scene via `simROS2`:

| CoppeliaSim Object | ROS 2 Topic | simROS2 API Call |
|--------------------|-------------|------------------|
| Diff drive script | `/{robot_id}/cmd_vel` | `simROS2.subscribe(...)` |
| Proximity sensor | `/{robot_id}/scan` | `simROS2.publish(...)` → `LaserScan` |
| Base object | `/{robot_id}/odom` | `simROS2.publish(...)` → `Odometry` |

The topic names, message types, and QoS profiles must be identical to the Gazebo backend contract above.

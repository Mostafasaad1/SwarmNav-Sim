# Interface Contract: Mission Executor Node

## ROS 2 Node API

### Node Name
`mission_executor` (typically pushed into a namespace like `/robot_0`)

### Type
`rclcpp_lifecycle::LifecycleNode`

### Parameters
| Name | Type | Default | Description |
|------|------|---------|-------------|
| `robot_id` | `string` | `"robot_0"` | The identifier for the robot, used to configure the blackboard. |
| `bt_xml_filename` | `string` | `""` | The absolute path to the `mission_tree.xml` behavior tree definition. |
| `tick_rate` | `double` | `10.0` | The frequency (in Hz) at which the behavior tree is ticked. |

### Subscriptions
None directly by the node wrapper. Subscriptions are created by the BT plugins registered in the factory (e.g., `/swarm/auction/result`, `/map`).

### Publishers
None directly by the node wrapper. Publishers are created by the BT plugins.

### Action Clients
None directly by the node wrapper. Action clients (e.g., `navigate_to_pose`) are created by the BT plugins.

### Services (Lifecycle)
As a standard managed node, it exposes the standard ROS 2 lifecycle services:
- `~/change_state` (`lifecycle_msgs/srv/ChangeState`)
- `~/get_state` (`lifecycle_msgs/srv/GetState`)
- `~/get_available_states` (`lifecycle_msgs/srv/GetAvailableStates`)
- `~/get_available_transitions` (`lifecycle_msgs/srv/GetAvailableTransitions`)

# Data Model: Mission Executor Node

The `mission_executor_node` is primarily a state machine executor rather than a data processing node. Its state is encapsulated by the ROS 2 Lifecycle state machine and the Behavior Tree instance.

## Entities

### `MissionExecutorNode` (Lifecycle Node)
Manages the execution environment for the behavior tree.

**Properties**:
- `tree_`: The instantiated `BT::Tree` representing the current execution state of the behavior tree.
- `factory_`: The `BT::BehaviorTreeFactory` used to register plugins and load the XML.
- `timer_`: A `rclcpp::TimerBase` that triggers the tree execution tick.
- `robot_id_`: String (e.g., "robot_0") used to namespace topics and actions.
- `bt_xml_filename_`: Path to the `mission_tree.xml` file.

**State Transitions**:
- **Unconfigured -> Inactive**: `on_configure()` loads the BT XML file and registers all custom node plugins into the factory.
- **Inactive -> Active**: `on_activate()` instantiates the tree from the factory and creates the timer to begin ticking.
- **Active -> Inactive**: `on_deactivate()` halts the tree execution and cancels the timer.
- **Inactive -> Unconfigured**: `on_cleanup()` destroys the tree and factory instances.

### Behavior Tree Blackboard
The shared data blackboard used by the BT nodes.

**Keys**:
- `node`: Pointer to the `rclcpp::Node` (used by custom plugins to create publishers/subscribers/action clients).
- `robot_id`: The string ID of the robot, allowing generic plugins to operate in the correct namespace.

# Research & Decisions: Mission Executor Node

## Technical Decisions

### Decision: Lifecycle Node Architecture
- **Decision**: Implement `mission_executor_node` as a `rclcpp_lifecycle::LifecycleNode`.
- **Rationale**: The BT logic heavily relies on Nav2 action servers (e.g., `navigate_to_pose`). If a standard node is used, the tree would begin ticking and calling actions before Nav2 is fully initialized, leading to connection timeouts and BT failures. By integrating with the lifecycle manager, the node will only transition to `Active` (and start ticking) when commanded by `nav2_lifecycle_manager` (which ensures Nav2 is ready).
- **Alternatives considered**: Standard `rclcpp::Node` with manual retries and `wait_for_action_server()` logic. Rejected due to complexity and non-standard behavior compared to the rest of the Nav2 ecosystem.

### Decision: Behavior Tree Execution Loop
- **Decision**: Use a ROS 2 `WallTimer` created inside `on_activate()` that ticks the tree at 10Hz, and destroy/pause it in `on_deactivate()`.
- **Rationale**: This fits perfectly with the ROS 2 executor paradigm, avoiding the need for manual threading or blocking `while(rclcpp::ok())` loops that would stall the node's ability to process callbacks (such as the action result callbacks needed by BT plugins).
- **Alternatives considered**: Spawning a separate `std::thread` to tick the tree. Rejected because it complicates ROS 2 lifecycle and executor management unnecessarily.

### Decision: Plugin Registration
- **Decision**: Statically link the custom BT plugins (`RunAuctionBT`, `FrontierDetectorBT`, `MapCoverageCheck`) instead of dynamically loading them via `.so` shared libraries.
- **Rationale**: The custom plugins are already in the `swarm_nav_coordination` package. Since `mission_executor_node` will also live in this package, static linking is simpler and avoids runtime plugin loader issues (e.g., missing library paths).
- **Alternatives considered**: Compiling plugins as shared libraries and loading them via `factory.registerFromPlugin()`. Rejected as it adds CMake complexity for internal plugins.

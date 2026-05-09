# Research: Complete Core Nodes — Technical Decisions

## 1. Recursive Mutex Deadlock Resolution Strategy

**Decision**: Remove the redundant `lock_guard` from callee functions (`computeOrcaVelocity`, `calculateBidCost`) since the caller already holds the lock.  
**Rationale**: Using `std::recursive_mutex` would hide design issues; the cleaner fix is to document lock ownership and ensure callees never re-acquire. This is the standard ROS 2 node threading pattern: callbacks acquire once, private helpers assume the lock is held.  
**Alternatives considered**: `std::recursive_mutex` (masks bugs, slight perf overhead), lock-free atomics (too complex for aggregate state), separating into multiple mutexes (risk of ABBA deadlocks).

## 2. ORCA Velocity Filter — Custom VO Implementation vs RVO2 Library

**Decision**: Implement a first-principles Velocity Obstacle (VO) algorithm in pure C++ without the external RVO2 library.  
**Rationale**: RVO2-ROS2 has no official Humble release and requires manual building from source. A custom VO implementation using half-plane constraints is pedagogically clearer and avoids dependency risk. The algorithm will compute truncated velocity obstacles for each neighbor and select the closest point to the preferred velocity outside all VO cones using linear programming.  
**Alternatives considered**: Linking against `RVO2` system library (dependency management burden), using only distance scaling (insufficient — does not guarantee collision freedom), ORCA via quadratic programming (more complex than needed for 3-5 robots).

### VO Algorithm Outline (to be implemented)

1. For each neighbor `j`, compute the truncated VO cone in velocity space:
   - `relative_pos = pos_j - pos_self`
   - `combined_radius = radius_self + radius_j`
   - Cone apex at `vel_j`, opening half-angle = `asin(combined_radius / |relative_pos|)`
   - Truncate at `time_horizon` boundary circle
2. For the preferred velocity `v_pref` (from Nav2):
   - If `v_pref` is outside ALL VO cones → safe, publish as-is
   - Otherwise → project `v_pref` onto the nearest VO boundary (half-plane projection)
3. Clamp the result to `max_linear_velocity` and `max_angular_velocity`

## 3. Obstacle Classification Decay Strategy

**Decision**: Implement a timestamp-based decay in `updateCosts()` using the obstacle's `classification` field to select the decay rate.  
**Rationale**: The costmap is updated every cycle (~10Hz). By storing the last-seen timestamp per obstacle and comparing against `now()`, we can compute time-since-last-update and apply exponential decay accordingly.  
**Decay Constants**: STATIC → no decay (permanent), SEMI_DYNAMIC → τ=5.0s, DYNAMIC → τ=2.0s.  
**Inflation Model**: Gaussian falloff `C = C_max * exp(-d² / (2*σ²))` where `σ = obstacle_radius + robot_radius + 0.2m`.

## 4. Graph Merge Strategy — Simplified Map Overlay

**Decision**: Implement map merging as occupancy grid overlay (cell-wise max) rather than full pose-graph optimization.  
**Rationale**: Full g2o/GTSAM-based pose graph optimization requires `mrg_slam` integration which is out of scope. A simplified approach overlays the neighbor's occupancy grid onto the local grid using a known relative transform from TF. This is sufficient to demonstrate map sharing and publish `/swarm/global_map`.  
**Alternatives considered**: Full ICP + g2o pipeline (requires external solver dependency), feature-based alignment (too complex without ORB/SURF), no merging at all (defeats the purpose of US1).

### Merge Algorithm

1. On rendezvous detection, subscribe to neighbor's `robot_X/map` topic
2. Look up the TF transform from neighbor's `map` frame to own `map` frame
3. For each cell in the neighbor's map, transform to own map coordinates
4. Apply cell-wise max occupancy: `merged[x][y] = max(local[x][y], transformed_neighbor[x][y])`
5. Publish the merged grid to `/swarm/global_map`

## 5. BehaviorTree v3 → v4 Migration

**Decision**: Migrate all BT nodes from `behaviortree_cpp_v3` to `behaviortree_cpp` (v4).  
**Rationale**: The spec requires BT.CPP v4 and the ROS 2 Humble `nav2_bt_navigator` ships with v4.  
**Key API Changes**:
- `BT::ConditionNode` → `BT::ConditionNode` (same name, different header `behaviortree_cpp/bt_factory.h`)
- `BT::SyncActionNode` → `BT::SyncActionNode` (header change)
- `BT::StatefulActionNode` → `BT::StatefulActionNode` (header change)
- `BT_REGISTER_NODES(factory)` → `BT_REGISTER_NODES(factory)` (same macro, different linker)
- CMakeLists: `find_package(behaviortree_cpp_v3)` → `find_package(behaviortree_cpp)`

## 6. NeighborState Aggregation Strategy

**Decision**: Each ORCA node publishes individual `NeighborState` messages; introduce a lightweight `neighbor_state_aggregator_node` that collects individual states and republishes them as a `NeighborStateArray`.  
**Rationale**: Having each robot publish its own state as a single `NeighborState` is the simplest pattern. The aggregator runs once per swarm and accumulates states received within a sliding time window, publishing the array at 10Hz. This avoids the type mismatch between publisher and subscriber.  
**Alternatives considered**: Having each robot maintain its own aggregation (duplicated logic, more complex), using a separate topic for individual states (complicates the subscription model).

## 7. Bid Cost Formula Implementation

**Decision**: Implement the spec formula: `cost = distance * 1.0 + travel_time * 0.5 + obstacle_density * 2.0 + task_switch_penalty * 0.3`.  
**Rationale**: Distance and travel time are computed from the Euclidean distance and an assumed constant velocity (0.5 m/s default). Obstacle density is queried from the local costmap's lethal cell count within a 5m radius of the frontier centroid. Task switch penalty is 1.0 if the robot is currently navigating to a different frontier, 0.0 otherwise.

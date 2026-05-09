# Implementation Plan: Complete All SwarmNav-Sim Core Node Implementations

**Branch**: `002-complete-core-nodes` | **Date**: 2026-05-09 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-complete-core-nodes/spec.md`

## Summary

Complete all stubbed and broken C++ node implementations across the SwarmNav-Sim workspace. This covers fixing 2 runtime-crashing mutex deadlocks, enabling the obstacle tracking pipeline, implementing spec-compliant auction/frontier formulas, adding classification-aware costmap decay, wiring BT nodes to live topics, implementing simplified graph merging, and correcting the launch file. The goal is to bring spec coverage from 50% to 100% of functional requirements.

## Technical Context

**Language/Version**: C++17 (ROS 2 Humble ament_cmake), Python 3.10 (ament_python)  
**Primary Dependencies**: rclcpp, nav2_costmap_2d, behaviortree_cpp (v4), tf2_ros, swarm_nav_msgs  
**Storage**: N/A (ROS 2 topics, in-memory state)  
**Testing**: Manual smoke tests via `ros2 topic pub/echo`, launch integration tests  
**Target Platform**: Ubuntu 22.04 (x86_64), ROS 2 Humble  
**Project Type**: Multi-package ROS 2 workspace (simulation)  
**Performance Goals**: All nodes run at ≥10 Hz without deadlocks for 10+ minutes  
**Constraints**: No machine learning, no neural networks, deterministic RNG only  
**Scale/Scope**: 3-5 robots, 6 ROS 2 packages, ~16 source files modified

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

Constitution file is an unpopulated template — no active principles defined. **GATE: PASS** (no constraints to violate).

## Project Structure

### Documentation (this feature)

```text
specs/002-complete-core-nodes/
├── plan.md              # This file
├── research.md          # Phase 0 output — 7 technical decisions
├── data-model.md        # Phase 1 output — formulas, decay rules, tie-breaking
├── quickstart.md        # Phase 1 output — smoke tests
├── contracts/
│   └── dds-qos-corrections.md  # QoS profile fixes
└── tasks.md             # Phase 2 output (via /speckit-tasks)
```

### Source Code (files to be modified)

```text
src/
├── swarm_nav_coordination/
│   ├── CMakeLists.txt                           # [MODIFY] BT v3→v4 dep
│   ├── src/
│   │   ├── auctioneer_node.cpp                  # [MODIFY] Fix deadlock, cost formula, tie-breaking, QoS
│   │   ├── frontier_detector_node.cpp           # [MODIFY] Utility formula, rename struct
│   │   └── bt_nodes/
│   │       ├── map_coverage_check.cpp           # [MODIFY] BT v4 headers
│   │       ├── frontier_detector_bt.cpp         # [MODIFY] BT v4 headers, wire to live topics
│   │       └── run_auction_bt.cpp               # [MODIFY] BT v4 headers, wire to live topics
│
├── swarm_nav_navigation/
│   ├── plugins/
│   │   ├── dynamic_obstacle_layer.hpp           # [MODIFY] Uncomment msg include
│   │   └── dynamic_obstacle_layer.cpp           # [MODIFY] Classification decay, Gaussian inflation
│   └── src/
│       ├── orca_velocity_filter_node.cpp        # [MODIFY] Fix deadlock, VO algorithm, rename struct, QoS
│       ├── obstacle_tracker_node.cpp            # [MODIFY] Uncomment publisher, include msg, publish
│       └── neighbor_state_aggregator_node.cpp   # [NEW] Aggregates individual NeighborState→Array
│
├── swarm_nav_slam/
│   └── src/
│       └── graph_merge_node.cpp                 # [MODIFY] Implement map overlay merge, publish global map
│
└── swarm_nav_bringup/
    └── launch/
        └── swarm.launch.py                      # [MODIFY] Fix num_robots, remove global_map_merger ref
```

**Structure Decision**: No new packages. All changes are modifications to existing files within the established 6-package workspace, plus one new node (`neighbor_state_aggregator_node`) in `swarm_nav_navigation`.

## Implementation Phases

### Phase A: Runtime Safety (US1) — Blocks everything

**Files**: `orca_velocity_filter_node.cpp`, `auctioneer_node.cpp`

1. Remove `std::lock_guard<std::mutex> lock(mutex_)` from `computeOrcaVelocity()` (L151) — caller already holds lock
2. Remove `std::lock_guard<std::mutex> lock(mutex_)` from `calculateBidCost()` (L291) — caller already holds lock
3. Replace `rand()` with `std::mt19937` seeded from `robot_id_` hash for determinism

### Phase B: Obstacle Pipeline (US2) — Critical data source

**Files**: `obstacle_tracker_node.cpp`

1. Add `#include "swarm_nav_msgs/msg/obstacle_array.hpp"`
2. Uncomment `obstacle_pub_` declaration and initialization
3. Implement `publishObstacles()` to create `ObstacleArray` from `obstacles_` map and publish

### Phase C: Auction & Frontier Formulas (US3) — Core intelligence

**Files**: `frontier_detector_node.cpp`, `auctioneer_node.cpp`

1. **Frontier utility**: Implement `calculateUtility()` as `size * 0.1 + info_gain * 0.5` where info_gain = count of unknown cells within 3m radius of centroid
2. **Bid cost**: Implement `calculateBidCost()` as `distance * 1.0 + travel_time * 0.5 + obstacle_density * 2.0 + task_switch * 0.3`
3. **Tie-breaking**: In `resolveAuction()`, add secondary sort by `robot_id` when costs are equal
4. **Struct rename**: Rename local `Frontier` → `FrontierData` in frontier_detector_node.cpp
5. **Struct rename**: Rename local `NeighborState` → `NeighborData` in orca_velocity_filter_node.cpp

### Phase D: Dynamic Obstacle Classification (US4) — Costmap quality

**Files**: `dynamic_obstacle_layer.hpp`, `dynamic_obstacle_layer.cpp`

1. Uncomment `#include "swarm_nav_msgs/msg/obstacle_array.hpp"` in the header
2. Add `last_seen` timestamp to `DynamicObstacle` struct
3. In `updateCosts()`: apply classification-based decay
   - STATIC: permanent, LETHAL cost
   - SEMI_DYNAMIC: linear decay over 5s
   - DYNAMIC: Gaussian `C = C_max * exp(-d² / (2*σ²))` with 2s decay
4. Add predictive trajectory inflation for DYNAMIC obstacles along velocity vector

### Phase E: BT Integration (US5) — Mission orchestration

**Files**: `frontier_detector_bt.cpp`, `run_auction_bt.cpp`, `map_coverage_check.cpp`, `CMakeLists.txt`

1. Change all `#include <behaviortree_cpp_v3/...>` to `#include <behaviortree_cpp/...>`
2. Change CMake `find_package(behaviortree_cpp_v3)` to `find_package(behaviortree_cpp)`
3. Wire `FrontierDetectorBT` to subscribe to `frontiers` topic and output real count
4. Wire `RunAuctionBT` to subscribe to `/swarm/auction/result` and output real assignment
5. Remove all hardcoded return values

### Phase F: Graph Merging (US6) — Map sharing

**Files**: `graph_merge_node.cpp`

1. Subscribe to each neighbor's `robot_X/map` topic dynamically on rendezvous
2. Implement `mergeGraphs()` as cell-wise max occupancy grid overlay using TF transforms
3. Publish merged map to `/swarm/global_map`

### Phase G: ORCA VO Algorithm (US1 enhancement) — Collision guarantee

**Files**: `orca_velocity_filter_node.cpp`

1. Replace simplified scaling in `computeOrcaVelocity()` with velocity obstacle computation:
   - Compute truncated VO cones for each neighbor
   - Project preferred velocity onto nearest safe point outside all VOs
   - Clamp to velocity limits
2. Uncomment QoS changes for `/swarm/neighbor_states` → `SensorDataQoS()`

### Phase H: Aggregator + Launch Fixes (US7) — Integration

**Files**: `neighbor_state_aggregator_node.cpp` (NEW), `swarm.launch.py`, `CMakeLists.txt`

1. Create `neighbor_state_aggregator_node.cpp`: subscribes to `/swarm/robot_state`, aggregates into `NeighborStateArray`, publishes to `/swarm/neighbor_states` at 10Hz
2. Update ORCA filter `state_pub_` to publish to `/swarm/robot_state` instead of `/swarm/neighbor_states`
3. Fix `swarm.launch.py`: respect `num_robots` parameter, remove `global_map_merger` reference, add aggregator node
4. Apply QoS corrections from `contracts/dds-qos-corrections.md` across all affected nodes

### Phase I: Polish — Cross-cutting

1. Apply QoS profiles from contract to all remaining topics
2. Verify deterministic RNG in all nodes
3. Update `IMPLEMENTATION_SUMMARY.md` and `README.md`

## Complexity Tracking

No constitution violations to justify.

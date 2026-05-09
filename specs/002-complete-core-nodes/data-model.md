# Data Model: Complete Core Nodes

**Extends**: `specs/001-swarm-nav-sim/data-model.md` — No schema changes to existing messages.

## Behavioral Additions to Existing Entities

### 1. Frontier Utility Formula (Updated)

The `utility` field in `Frontier.msg` is now computed as:

```
utility = frontier_size * 0.1 + information_gain * 0.5
```

Where `information_gain` = count of unknown cells (-1) within a 3m radius of the frontier centroid.

### 2. Auction Bid Cost Formula (Updated)

The `bid_cost` field in `AuctionBid.msg` is computed as:

```
bid_cost = distance * 1.0 + travel_time * 0.5 + obstacle_density * 2.0 + task_switch_penalty * 0.3
```

| Component | Source | Computation |
|-----------|--------|-------------|
| `distance` | Euclidean from current odom pose to frontier centroid | `sqrt(dx² + dy²)` |
| `travel_time` | Distance divided by nominal speed | `distance / 0.5` (m/s) |
| `obstacle_density` | Fraction of lethal cells within 5m radius of centroid on local costmap | `lethal_count / total_cells_in_radius` |
| `task_switch_penalty` | 1.0 if robot is currently navigating to a different frontier, else 0.0 | Binary flag |

### 3. Obstacle Decay Classification (New Behavioral Contract)

The `classification` field in `Obstacle.msg` maps to costmap behavior:

| Classification | Value | Decay Time Constant (τ) | Inflation Model |
|----------------|-------|--------------------------|-----------------|
| STATIC | 0 | ∞ (permanent) | Fixed radius circle, LETHAL cost |
| SEMI_DYNAMIC | 1 | 5.0 seconds | Linear falloff from center |
| DYNAMIC | 2 | 2.0 seconds | Gaussian: `C = C_max * exp(-d² / (2*σ²))`, σ = obstacle_radius + robot_radius + 0.2m |

### 4. Vickrey Auction Tie-Breaking Rule (New)

When two bids have identical `bid_cost`, the winner is the robot with the lexicographically **lower** `robot_id` string.

### 5. New Entity: NeighborStateAggregator (Runtime Concept)

A lightweight node that collects individual `NeighborState` messages on `/swarm/neighbor_states` and periodically publishes a `NeighborStateArray` containing all states received within a 500ms sliding window. This resolves the type mismatch between ORCA publishers (individual states) and subscribers (array).

## State Transitions (Unchanged)

See `specs/001-swarm-nav-sim/data-model.md` — the IDLE → ANNOUNCE → COLLECT_BIDS → RESOLVE cycle is unchanged.

# Data Model: SwarmNav-Sim

## Core Entities

### 1. Frontier (`swarm_nav_msgs/msg/Frontier`)
Represents an unexplored boundary in the map.
- **`centroid`** (`geometry_msgs/Point`): 3D coordinate of the frontier center.
- **`size`** (`float32`): Number of cells comprising the frontier.
- **`utility`** (`float32`): Estimated information gain.
- **`frontier_id`** (`string`): Unique UUID for tracking.

### 2. Tracked Obstacle (`swarm_nav_msgs/msg/Obstacle`)
Represents a dynamic or semi-dynamic entity in the environment.
- **`id`** (`string`): Unique identifier (e.g., "human_1", "forklift_2").
- **`pose`** (`geometry_msgs/Pose`): Current position and orientation.
- **`velocity`** (`geometry_msgs/Twist`): Current linear and angular velocity.
- **`radius`** (`float32`): Collision radius.
- **`classification`** (`uint8`): Enum for STATIC (0), SEMI_DYNAMIC (1), DYNAMIC (2).

### 3. Neighbor State (`swarm_nav_msgs/msg/NeighborState`)
Broadcasted by each robot for ORCA collision avoidance.
- **`robot_id`** (`string`): Namespace ID (e.g., "robot_1").
- **`pose`** (`geometry_msgs/Pose`): Current localized pose.
- **`velocity`** (`geometry_msgs/Twist`): Current velocity.
- **`radius`** (`float32`): Footprint radius.

### 4. Auction Messages (`swarm_nav_msgs/msg/`)
- **`AuctionAnnounce`**: Contains an array of `Frontier` objects and the `auction_id`.
- **`AuctionBid`**: Contains `auction_id`, `frontier_id`, `robot_id`, and `bid_cost` (float32).
- **`AuctionResult`**: Contains `auction_id`, `frontier_id`, `winner_id`, and `winning_bid` (float32).

## State Transitions (Task Allocation)

1. **STATE_IDLE**: Robot is waiting or executing a task.
   - *Transition*: New frontiers detected -> STATE_ANNOUNCE
2. **STATE_ANNOUNCE**: Robot broadcasts its frontiers.
   - *Transition*: Frontiers broadcasted -> STATE_COLLECT_BIDS
3. **STATE_COLLECT_BIDS**: Waiting for neighbor bids (500ms timeout).
   - *Transition*: Timeout reached or all bids in -> STATE_RESOLVE
4. **STATE_RESOLVE**: Vickrey auction logic resolves winner.
   - *Transition*: Winner assigned -> STATE_IDLE

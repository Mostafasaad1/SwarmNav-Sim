# SwarmNav-Sim Tuning Guide

This guide provides recommendations for tuning the SwarmNav-Sim system parameters to achieve optimal performance.

## SLAM Parameters

### mrg_slam Configuration (`config/mrg_slam_multirobot.yaml`)

**Keyframe Generation**
- `keyframe_delta_trans`: 1.0m - Increase for sparser maps, decrease for denser mapping
- `keyframe_delta_angle`: 0.5 rad - Adjust based on rotation frequency
- `keyframe_delta_time`: 1.0s - Minimum time between keyframes

**Loop Closure**
- `loop_closure_distance_thresh`: 10.0m - Maximum distance for loop detection
- `loop_closure_fitness_score_thresh`: 0.5 - Lower = stricter matching

**Multi-Robot Graph Merging**
- `rendezvous_distance`: 3.0m - Distance threshold for graph exchange
- Increase if robots rarely meet, decrease for more frequent merging

## Navigation Parameters

### TEB Local Planner (`config/robot_nav2.yaml`)

**Velocity Limits**
- `max_vel_x`: 0.5 m/s - Maximum forward velocity
- `max_vel_theta`: 1.0 rad/s - Maximum rotational velocity
- Reduce for safer navigation in cluttered environments

**Obstacle Avoidance**
- `min_obstacle_dist`: 0.3m - Minimum distance to obstacles
- `inflation_dist`: 0.6m - Inflation radius around obstacles
- `dynamic_obstacle_inflation_dist`: 0.6m - Extra inflation for moving obstacles

**Optimization Weights**
- `weight_obstacle`: 100 - Penalty for proximity to obstacles
- `weight_dynamic_obstacle`: 10 - Penalty for dynamic obstacles
- `weight_optimaltime`: 1 - Preference for time-optimal paths

### Dynamic Obstacle Layer

**Prediction**
- `prediction_time`: 2.0s - How far ahead to predict obstacle positions
- Increase for faster-moving obstacles

**Inflation**
- `inflation_radius`: 1.0m - Safety margin around predicted positions

## Coordination Parameters

### Frontier Detection

**Detection Thresholds**
- `min_frontier_size`: 10 cells - Minimum frontier size to consider
- Increase to focus on larger unexplored areas

### Vickrey Auction

**Timing**
- `bid_timeout_ms`: 500ms - Time to collect bids
- Increase for larger swarms or high-latency networks

## ORCA Collision Avoidance

**Safety Parameters**
- `robot_radius`: 0.25m - Robot footprint radius
- `time_horizon`: 2.0s - Planning horizon for collision avoidance
- `max_neighbors`: 10 - Maximum neighbors to consider

## Performance Tuning

### For Higher Coverage Speed
1. Increase `max_vel_x` to 0.7 m/s
2. Reduce `min_frontier_size` to 5 cells
3. Increase `bid_timeout_ms` to 300ms for faster decisions

### For Better SLAM Accuracy
1. Decrease `keyframe_delta_trans` to 0.5m
2. Increase `loop_closure_fitness_score_thresh` to 0.7
3. Reduce `max_vel_x` to 0.3 m/s

### For Zero Collisions
1. Increase `min_obstacle_dist` to 0.5m
2. Increase `inflation_dist` to 0.8m
3. Increase `time_horizon` to 3.0s
4. Reduce `max_vel_x` to 0.4 m/s

## Troubleshooting

### Robots Not Merging Maps
- Check `rendezvous_distance` - may be too small
- Verify `/mrg_slam/shared_graph` topic is publishing
- Check ROS 2 Discovery Server is running

### Frequent Collisions
- Increase `min_obstacle_dist` and `inflation_dist`
- Reduce `max_vel_x` and `max_vel_theta`
- Verify ORCA velocity filter is active

### Poor Coverage
- Reduce `min_frontier_size` to detect smaller frontiers
- Increase `max_vel_x` for faster exploration
- Check frontier detector is publishing to `/frontiers`

### High SLAM Error (ATE > 0.3m)
- Decrease `keyframe_delta_trans` for denser mapping
- Verify loop closure is enabled
- Check sensor data quality (LiDAR scan rate, range)

## Benchmark Target Values

- **Coverage**: ≥ 95% in ≤ 10 minutes
- **SLAM ATE RMSE**: < 0.3m
- **Collisions**: 0
- **Real-time Factor**: ≥ 1.0

## Configuration Files Reference

- SLAM: `src/swarm_nav_bringup/config/mrg_slam_multirobot.yaml`
- Navigation: `src/swarm_nav_bringup/config/robot_nav2.yaml`
- Discovery: `src/swarm_nav_bringup/config/fastdds_discovery.xml`
- Behavior Tree: `src/swarm_nav_bringup/config/behavior_trees/mission_tree.xml`

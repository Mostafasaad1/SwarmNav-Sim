# Data Model: Deep Analysis & Tuning Suite

## Entities

### `BenchmarkScenario`
Defines a specific configuration to run in the simulator.
- `id` (str): Unique identifier for the scenario.
- `num_robots` (int): Number of robots in the swarm.
- `map` (str): Path to the map/world file.
- `timeout_sec` (int): Maximum simulation time allowed before aborting.
- `parameters` (dict): Parameter overrides for the ORCA filter, Nav2, etc.

### `PerformanceReport`
The resulting data generated from a `BenchmarkScenario` run.
- `scenario_id` (str): Foreign key to the scenario.
- `duration_sec` (float): Actual time taken.
- `collision_count` (int): Total collisions across the swarm.
- `coverage_percentage` (float): Explored area percentage.
- `cpu_avg_percent` (float): Average CPU usage during the run.
- `mem_avg_mb` (float): Average memory usage during the run.
- `rtf_avg` (float): Average Real Time Factor.
- `status` (str): "SUCCESS", "TIMEOUT", or "CRASH".

### `TuneSpace`
Defines the bounds for Bayesian Optimization via Optuna.
- `parameter_name` (str): Name of the ROS 2 parameter.
- `type` (str): "float", "int", or "categorical".
- `min` (float/int): Minimum bound.
- `max` (float/int): Maximum bound.
- `choices` (list): Valid options if categorical.

## State Transitions
- **Benchmark Run**: `IDLE` -> `LAUNCHING` -> `RUNNING` -> `TEARDOWN` -> `COMPLETED` (or `FAILED` if crashed/timeout).

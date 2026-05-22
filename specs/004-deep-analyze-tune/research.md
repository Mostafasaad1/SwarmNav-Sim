# Phase 0: Outline & Research

## Decision 1: Bayesian Optimization Library
**Decision**: Use `optuna` for Bayesian Optimization.
**Rationale**: Optuna is the industry standard for hyperparameter tuning in Python. It provides an efficient Define-by-Run API, prunes unpromising trials automatically, and easily handles both continuous and discrete parameter spaces. It also has built-in visualization tools for parameter importance.
**Alternatives considered**: 
- `scikit-optimize` (skopt): Less actively maintained compared to Optuna.
- `hyperopt`: Older API, slightly harder to integrate with modern ROS 2 Python nodes compared to Optuna.

## Decision 2: System Metrics Collection
**Decision**: Use `psutil` in a lightweight Python node.
**Rationale**: `psutil` provides cross-platform system monitoring (CPU, memory, disk, network) with minimal overhead. We can run a dedicated `system_metrics.py` node that polls `psutil` at a low frequency (e.g., 1 Hz) and appends to the scenario's results without interfering with the simulation.
**Alternatives considered**: 
- Using standard Linux tools (`top`, `pidstat`) via `subprocess`: Harder to parse safely and elegantly in Python.
- ROS 2 `system_metrics_collector` packages: Too heavyweight or unmaintained in ROS 2 Humble.

## Decision 3: Headless Simulator Execution
**Decision**: Pass `headless:=true` to the Gazebo Fortress launch configuration.
**Rationale**: Gazebo Fortress (`ros_gz_sim`) supports launching the server without the GUI client (`gz sim -s`). This prevents OpenGL rendering overhead, allowing the simulation to run faster (higher RTF) and consume drastically less GPU/CPU resources during unattended testing.
**Alternatives considered**:
- Xvfb (Virtual Framebuffer): Unnecessary since Gazebo naturally supports headless mode.

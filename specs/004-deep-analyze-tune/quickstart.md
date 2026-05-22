# Quickstart: Deep Analysis & Tuning Suite

## Prerequisites
Ensure you have the required Python packages installed:
```bash
pip install optuna psutil pandas
```

## Running a Benchmark Suite
To run a predefined benchmark suite from `scenarios.yaml`:
```bash
ros2 run swarm_nav_evaluation benchmark_runner --ros-args -p config_file:=src/swarm_nav_evaluation/config/scenarios.yaml
```
This will run headlessly by default. If you need to debug a run with the GUI:
```bash
ros2 run swarm_nav_evaluation benchmark_runner --ros-args -p config_file:=src/swarm_nav_evaluation/config/scenarios.yaml -p gui:=true
```

## Running the Optuna Tuner
To run the automated Bayesian Optimization to find optimal parameters based on the bounds defined in `tune_space.yaml`:
```bash
ros2 run swarm_nav_evaluation bayesian_tuner --ros-args -p tune_config:=src/swarm_nav_evaluation/config/tune_space.yaml
```

Reports and results will be saved to `~/.ros/swarm_nav_results/`.

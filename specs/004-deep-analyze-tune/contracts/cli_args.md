# Node Parameter Contracts

## `benchmark_runner` Node

- `config_file` (string, required): Path to the `scenarios.yaml` configuration file.
- `gui` (bool, default: false): Whether to launch Gazebo with the GUI client.
- `output_dir` (string, default: `~/.ros/swarm_nav_results`): Directory to write the CSV/JSON reports.

## `bayesian_tuner` Node

- `tune_config` (string, required): Path to the `tune_space.yaml` configuration file.
- `trials` (int, default: 50): Number of Optuna trials to run.
- `gui` (bool, default: false): Whether to launch Gazebo with the GUI client.
- `output_dir` (string, default: `~/.ros/swarm_nav_results`): Directory to write the tuning results and plots.

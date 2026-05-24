#!/usr/bin/env python3
"""Bayesian Optimization tuner for SwarmNav-Sim using Optuna.

Automatically tunes system parameters to find the most efficient configuration.
"""

import argparse
import os
import sys
import tempfile
from datetime import datetime
import time
from typing import Any, Dict, List, Optional

import yaml

try:
    import optuna
    from optuna.samplers import TPESampler
    OPTUNA_AVAILABLE = True
except ImportError:
    OPTUNA_AVAILABLE = False
    print('Warning: optuna not available. Bayesian tuning will not work.')

from swarm_nav_evaluation.benchmark_runner import BenchmarkRunner
from swarm_nav_evaluation.report_utils import write_csv_report, write_json_report


class BayesianTuner:
    """Bayesian Optimization tuner using Optuna."""

    def __init__(
        self,
        tune_config: str,
        trials: int = 50,
        output_dir: str = None,
        gui: bool = False,
    ):
        """Initialize the tuner with config, number of trials, and output dir."""
        self.tune_config = tune_config
        self.trials = trials
        self.output_dir = output_dir or os.path.expanduser(
            '~/.ros/swarm_nav_results'
        )
        self.gui = gui
        self.config: Dict = {}
        self.study: Optional[Any] = None
        self.results: List[Dict] = []

    def load_config(self) -> None:
        """Load tuning configuration from YAML."""
        with open(self.tune_config, 'r') as f:
            self.config = yaml.safe_load(f)
        print(f'Loaded tuning config from {self.tune_config}')

    def create_study(self) -> None:
        """Create an Optuna study with TPE sampler."""
        if not OPTUNA_AVAILABLE:
            raise ImportError('optuna is required for Bayesian tuning')

        sampler = TPESampler(n_startup_trials=10)
        self.study = optuna.create_study(
            direction=self.config.get('objective', {}).get(
                'direction', 'minimize'
            ),
            sampler=sampler,
        )
        print(f'Created Optuna study with {self.trials} trials')

    def objective(self, trial) -> float:
        """Optuna objective function: run a benchmark with suggested parameters."""
        params = self._suggest_parameters(trial)
        print(f'\nTrial {trial.number}: {params}')

        # Run benchmark with these parameters
        result = self._run_benchmark(params)

        # Store result
        self.results.append({
            'trial': trial.number,
            'params': params,
            'result': result,
        })

        # Return the metric to optimize
        metric_key = self.config.get('objective', {}).get(
            'metric', 'exploration_time_sec'
        )
        return result.get(metric_key, float('inf'))

    def _suggest_parameters(self, trial) -> Dict:
        """Suggest parameters based on tuning space configuration."""
        params = {}
        for param_config in self.config.get('parameters', []):
            name = param_config['name']
            param_type = param_config['type']

            if param_type == 'float':
                params[name] = trial.suggest_float(
                    name, param_config['min'], param_config['max']
                )
            elif param_type == 'int':
                params[name] = trial.suggest_int(
                    name, param_config['min'], param_config['max']
                )
            elif param_type == 'categorical':
                params[name] = trial.suggest_categorical(
                    name, param_config.get('choices', [])
                )

        return params

    def _run_benchmark(self, params: Dict) -> Dict:
        """Run a single benchmark trial with the given Optuna-suggested params.

        Creates a temporary scenarios.yaml from the base config and the
        suggested parameters, then runs it through BenchmarkRunner and
        returns the domain metrics result.
        """
        base_scenario = self.config.get('base_scenario', {})

        # Build a per-trial scenario config
        trial_scenario = {
            'id': f'optuna_trial_{int(time.time())}',
            'name': 'Optuna Trial',
            'num_robots': base_scenario.get('num_robots', 3),
            'map': base_scenario.get('map', 'warehouse_gz.sdf'),
            'timeout_sec': base_scenario.get('timeout_sec', 120),
            'parameters': params,
        }
        trial_config = {'scenarios': [trial_scenario]}

        # Write to a temp YAML and run through BenchmarkRunner
        with tempfile.NamedTemporaryFile(
            mode='w', suffix='.yaml', delete=False
        ) as f:
            yaml.dump(trial_config, f)
            tmp_config_path = f.name

        trial_output_dir = os.path.join(
            self.output_dir, 'trials', trial_scenario['id']
        )

        try:
            runner = BenchmarkRunner(
                config_file=tmp_config_path,
                output_dir=trial_output_dir,
                gui=self.gui,
            )
            results = runner.run_all()
            if results:
                return results[0]
            return {'exploration_time_sec': float('inf')}
        finally:
            os.unlink(tmp_config_path)

    def run(self) -> Dict:
        """Run the Bayesian optimization and return the final report."""
        self.load_config()
        self.create_study()

        print(f'\nStarting Bayesian Optimization with {self.trials} trials...')
        self.study.optimize(self.objective, n_trials=self.trials)

        # Generate report
        return self._generate_report()

    def _generate_report(self) -> Dict:
        """Generate tuning report and write JSON/CSV outputs."""
        os.makedirs(self.output_dir, exist_ok=True)

        best_params = self.study.best_params
        best_value = self.study.best_value

        report = {
            'tuning_timestamp': datetime.now().isoformat(),
            'trials': self.trials,
            'best_params': best_params,
            'best_value': best_value,
            'all_trials': [
                {
                    'trial': r['trial'],
                    'params': r['params'],
                    'result': r['result'],
                }
                for r in self.results
            ],
        }

        # Write JSON report
        json_path = os.path.join(self.output_dir, 'tuning_report.json')
        write_json_report(report, json_path)

        # Write CSV report
        csv_rows = []
        for r in self.results:
            row = {'trial': r['trial']}
            row.update(r['params'])
            row.update(r['result'])
            csv_rows.append(row)
        csv_path = os.path.join(self.output_dir, 'tuning_report.csv')
        write_csv_report(csv_rows, csv_path)

        separator = '=' * 60
        print(f'\n{separator}')
        print('Tuning Complete!')
        print(f'Best Params: {best_params}')
        print(f'Best Value:  {best_value}')
        print(f'Report saved to: {json_path}')
        print(separator)

        return report


def main(args=None):
    """Entry point for the Bayesian tuner CLI."""
    parser = argparse.ArgumentParser(
        description='SwarmNav-Sim Bayesian Tuner'
    )
    parser.add_argument(
        '--config', '-c',
        default='src/swarm_nav_evaluation/config/tune_space.yaml',
        help='Path to tuning space YAML config file',
    )
    parser.add_argument(
        '--trials', '-t',
        type=int,
        default=50,
        help='Number of Optuna trials to run',
    )
    parser.add_argument(
        '--output-dir', '-o',
        default='~/.ros/swarm_nav_results',
        help='Directory to write tuning results',
    )
    parser.add_argument(
        '--gui',
        action='store_true',
        help='Launch simulator with GUI',
    )

    parsed_args = parser.parse_args(args)

    if not OPTUNA_AVAILABLE:
        print(
            'Error: optuna is required for Bayesian tuning. '
            'Install with: pip install optuna'
        )
        sys.exit(1)

    output_dir = os.path.expanduser(parsed_args.output_dir)
    tuner = BayesianTuner(
        tune_config=parsed_args.config,
        trials=parsed_args.trials,
        output_dir=output_dir,
        gui=parsed_args.gui,
    )

    tuner.run()


if __name__ == '__main__':
    main()

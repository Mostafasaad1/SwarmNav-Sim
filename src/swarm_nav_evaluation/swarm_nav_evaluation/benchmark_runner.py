#!/usr/bin/env python3
"""Benchmark runner for SwarmNav-Sim.

Executes a suite of predefined benchmark scenarios, collects metrics,
and generates consolidated performance reports.
"""

import argparse
import json
import os
import signal
import subprocess
from copy import deepcopy
import time
from datetime import datetime
from typing import Dict, List, Optional

from swarm_nav_evaluation.report_utils import (
    generate_sweep_report,
    write_csv_report,
    write_json_report,
)
from swarm_nav_evaluation.system_metrics import SystemMetricsCollector


class BenchmarkRunner:
    """Runs a suite of benchmark scenarios and generates reports."""

    def __init__(self, config_file: str, output_dir: str, gui: bool = False):
        """Initialize the runner with config file, output dir, and GUI flag."""
        self.config_file = config_file
        self.output_dir = output_dir
        self.gui = gui
        self.scenarios: List[Dict] = []
        self.results: List[Dict] = []
        self.sweep_results: List[Dict] = []
        self._current_process: Optional[subprocess.Popen] = None
        self._abort = False

        # Setup signal handlers for graceful shutdown
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, signum, frame):
        """Handle shutdown signals."""
        print(f'\nReceived signal {signum}. Aborting benchmark...')
        self._abort = True
        if self._current_process:
            self._current_process.terminate()

    def load_scenarios(self) -> None:
        """Load scenarios from YAML config file."""
        with open(self.config_file, 'r') as f:
            config = yaml.safe_load(f)
        self.scenarios = config.get('scenarios', [])
        print(f'Loaded {len(self.scenarios)} scenarios from {self.config_file}')

    def load_sweep_config(self) -> List[Dict]:
        """Parse and generate all permutations from the sweep config."""
        with open(self.config_file, 'r') as f:
            config = yaml.safe_load(f)
        sweep_configs = config.get('sweep', [])
        if not sweep_configs:
            return []

        scenario_map = {s['id']: s for s in self.scenarios}
        permutations = []

        for sweep in sweep_configs:
            base = scenario_map.get(sweep.get('base_scenario'))
            if not base:
                print(
                    f"Warning: base scenario '{sweep.get('base_scenario')}' "
                    'not found'
                )
                continue

            param_name = sweep['parameter']
            for value in sweep['values']:
                scenario = deepcopy(base)
                scenario['id'] = f'{base["id"]}_{param_name}={value}'
                scenario['name'] = f'{base["name"]} ({param_name}={value})'
                if 'parameters' not in scenario:
                    scenario['parameters'] = {}
                scenario['parameters'][param_name] = value
                permutations.append(scenario)

        print(
            f'Generated {len(permutations)} sweep permutations '
            f'from {len(sweep_configs)} sweeps'
        )
        return permutations

    def run_sweep(self) -> List[Dict]:
        """Run parameter sweep and generate sensitivity report."""
        self.load_scenarios()
        sweep_scenarios = self.load_sweep_config()
        if not sweep_scenarios:
            print('No sweep configurations found.')
            return []

        os.makedirs(self.output_dir, exist_ok=True)
        self.sweep_results = []

        for scenario in sweep_scenarios:
            if self._abort:
                break
            result = self.run_scenario(scenario)
            self.sweep_results.append(result)

        sweep_dir = os.path.join(self.output_dir, 'sweep')
        report_paths = generate_sweep_report(self.sweep_results, sweep_dir)
        print(f"Sweep report saved: {report_paths.get('csv', 'N/A')}")

        return self.sweep_results

    def run_all(self) -> List[Dict]:
        """Run all scenarios sequentially."""
        self.load_scenarios()
        os.makedirs(self.output_dir, exist_ok=True)

        for scenario in self.scenarios:
            if self._abort:
                print('Benchmark aborted by user.')
                break

            result = self.run_scenario(scenario)
            self.results.append(result)

            # Fail-fast: abort on crash/timeout
            if result.get('status') in ('CRASH', 'TIMEOUT'):
                print(
                    f'FAIL-FAST: Scenario {scenario["id"]} '
                    f"{result['status']}. Aborting suite."
                )
                break

        # Generate consolidated report
        self._generate_consolidated_report()
        return self.results

    def run_scenario(self, scenario: Dict) -> Dict:
        """Run a single benchmark scenario."""
        scenario_id = scenario['id']
        sep = '=' * 60
        print(f'\n{sep}')
        print(f"Running scenario: {scenario_id} - {scenario.get('name', '')}")
        print(sep)

        # Start system metrics collection
        metrics = SystemMetricsCollector(interval=1.0)
        metrics.start()

        start_time = time.time()
        status = 'SUCCESS'
        error_msg = None

        try:
            self._launch_scenario(scenario)
        except subprocess.TimeoutExpired:
            status = 'TIMEOUT'
            error_msg = (
                f"Scenario exceeded timeout of "
                f"{scenario.get('timeout_sec', 300)}s"
            )
        except Exception as e:
            status = 'CRASH'
            error_msg = str(e)
        finally:
            metrics.stop()

        duration = time.time() - start_time

        # Output directory for this scenario
        scenario_output_dir = os.path.join(self.output_dir, scenario_id)
        os.makedirs(scenario_output_dir, exist_ok=True)

        # Read domain metrics from evaluation node JSON outputs
        domain_metrics = self._read_domain_metrics(scenario_output_dir)

        result = {
            'scenario_id': scenario_id,
            'scenario_name': scenario.get('name', scenario_id),
            'duration_sec': duration,
            'status': status,
            'error': error_msg,
            'cpu_avg_percent': metrics.get_average_cpu(),
            'mem_avg_mb': metrics.get_average_memory_mb(),
            'peak_cpu_percent': metrics.get_peak_cpu(),
            'peak_memory_mb': metrics.get_peak_memory_mb(),
            'timestamp': datetime.now().isoformat(),
            **domain_metrics,
        }

        # Write individual scenario report
        report_path = os.path.join(scenario_output_dir, 'report.json')
        write_json_report(result, report_path)

        print(f'Scenario {scenario_id} completed: {status} in {duration:.1f}s')
        return result

    def _read_domain_metrics(self, scenario_output_dir: str) -> Dict:
        """Read domain metrics from evaluation node JSON output files."""
        domain = {
            'coverage_percentage': 0.0,
            'collision_count': 0,
            'exploration_time_sec': 0.0,
            'map_quality': 0.0,
        }

        # coverage_results.json from coverage_evaluator node
        coverage_path = os.path.join(scenario_output_dir, 'coverage_results.json')
        if os.path.exists(coverage_path):
            try:
                with open(coverage_path, 'r') as f:
                    data = json.load(f)
                domain['coverage_percentage'] = data.get(
                    'coverage_percentage', 0.0
                )
                domain['exploration_time_sec'] = data.get(
                    'exploration_time_sec', 0.0
                )
            except (json.JSONDecodeError, OSError):
                pass

        # collision_results.json from collision_monitor node
        collision_path = os.path.join(
            scenario_output_dir, 'collision_results.json'
        )
        if os.path.exists(collision_path):
            try:
                with open(collision_path, 'r') as f:
                    data = json.load(f)
                domain['collision_count'] = data.get('total_collisions', 0)
            except (json.JSONDecodeError, OSError):
                pass

        # slam_metrics.json from slam_metrics node
        slam_path = os.path.join(scenario_output_dir, 'slam_metrics.json')
        if os.path.exists(slam_path):
            try:
                with open(slam_path, 'r') as f:
                    data = json.load(f)
                domain['map_quality'] = data.get('avg_ate', 0.0)
            except (json.JSONDecodeError, OSError):
                pass

        return domain

    def _launch_scenario(self, scenario: Dict) -> None:
        """Launch a scenario using ROS 2 launch."""
        timeout_sec = scenario.get('timeout_sec', 300)
        num_robots = scenario.get('num_robots', 3)
        scenario_output_dir = os.path.join(self.output_dir, scenario['id'])
        params = scenario.get('parameters', {})

        # Build launch command with parameter overrides
        cmd = [
            'ros2', 'launch', 'swarm_nav_evaluation', 'benchmark.launch.py',
            f'num_robots:={num_robots}',
            f'duration:={timeout_sec}',
            f'output_dir:={scenario_output_dir}',
            f'gui:={"true" if self.gui else "false"}',
        ]

        for key, value in params.items():
            cmd.append(f'{key}:={value}')

        print(f"Launching: {' '.join(cmd)}")
        self._current_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # Wait with a generous buffer beyond the scenario timeout
        try:
            stdout, stderr = self._current_process.communicate(
                timeout=timeout_sec + 30
            )
            if self._current_process.returncode != 0:
                raise RuntimeError(f'Launch failed: {stderr}')
        except subprocess.TimeoutExpired:
            self._current_process.kill()
            raise
        finally:
            self._current_process = None

    def _generate_consolidated_report(self) -> None:
        """Generate a consolidated report from all scenario results."""
        report = {
            'benchmark_timestamp': datetime.now().isoformat(),
            'total_scenarios': len(self.scenarios),
            'completed_scenarios': len(self.results),
            'scenarios': self.results,
        }

        # JSON report
        json_path = os.path.join(self.output_dir, 'consolidated_report.json')
        write_json_report(report, json_path)

        # CSV report
        csv_path = os.path.join(self.output_dir, 'consolidated_report.csv')
        write_csv_report(self.results, csv_path)

        separator = '=' * 60
        print(f'\n{separator}')
        print('Consolidated report saved to:')
        print(f'  JSON: {json_path}')
        print(f'  CSV:  {csv_path}')
        print(separator)


def main(args=None):
    """Entry point for the benchmark runner CLI."""
    parser = argparse.ArgumentParser(
        description='SwarmNav-Sim Benchmark Runner'
    )
    parser.add_argument(
        '--config', '-c',
        default='src/swarm_nav_evaluation/config/scenarios.yaml',
        help='Path to scenarios YAML config file',
    )
    parser.add_argument(
        '--output-dir', '-o',
        default='~/.ros/swarm_nav_results',
        help='Directory to write reports',
    )
    parser.add_argument(
        '--gui',
        action='store_true',
        help='Launch simulator with GUI',
    )
    parser.add_argument(
        '--sweep',
        action='store_true',
        help='Run parameter sweep instead of standard benchmark suite',
    )

    parsed_args = parser.parse_args(args)

    output_dir = os.path.expanduser(parsed_args.output_dir)
    runner = BenchmarkRunner(
        config_file=parsed_args.config,
        output_dir=output_dir,
        gui=parsed_args.gui,
    )

    if parsed_args.sweep:
        runner.run_sweep()
    else:
        runner.run_all()


if __name__ == '__main__':
    main()

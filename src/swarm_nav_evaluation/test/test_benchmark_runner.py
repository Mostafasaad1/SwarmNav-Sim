#!/usr/bin/env python3
"""Tests for the benchmark runner."""

import os
import sys
import tempfile
import unittest

# Add the package source to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'swarm_nav_evaluation'))

from benchmark_runner import BenchmarkRunner  # noqa: E402
from report_utils import append_csv_report, write_csv_report, write_json_report  # noqa: E402
from system_metrics import SystemMetricsCollector  # noqa: E402


class TestBenchmarkRunner(unittest.TestCase):
    """Test cases for BenchmarkRunner."""

    def setUp(self):
        """Set up test fixtures."""
        self.temp_dir = tempfile.mkdtemp()
        self.config_file = os.path.join(self.temp_dir, 'test_scenarios.yaml')
        with open(self.config_file, 'w') as f:
            f.write("""
scenarios:
  - id: "test_scenario"
    name: "Test Scenario"
    num_robots: 2
    map: "test_map.sdf"
    timeout_sec: 10
    parameters:
      max_linear_velocity: 0.5

sweep:
  - parameter: max_linear_velocity
    values: [0.3, 0.5, 0.8]
    base_scenario: test_scenario
""")

    def tearDown(self):
        """Clean up test fixtures."""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_load_scenarios(self):
        """Test loading scenarios from YAML."""
        runner = BenchmarkRunner(self.config_file, self.temp_dir)
        runner.load_scenarios()
        self.assertEqual(len(runner.scenarios), 1)
        self.assertEqual(runner.scenarios[0]['id'], 'test_scenario')

    def test_run_scenario_mock(self):
        """Test running a scenario with mocked launch."""
        runner = BenchmarkRunner(self.config_file, self.temp_dir)
        runner.load_scenarios()
        scenario = runner.scenarios[0]

        # Mock the _launch_scenario to avoid actual ROS launch
        def mock_launch(scenario):
            pass

        runner._launch_scenario = mock_launch
        result = runner.run_scenario(scenario)

        self.assertEqual(result['scenario_id'], 'test_scenario')
        self.assertEqual(result['status'], 'SUCCESS')
        self.assertIn('duration_sec', result)
        self.assertIn('cpu_avg_percent', result)


class TestReportUtils(unittest.TestCase):
    """Test cases for report utilities."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()

    def tearDown(self):
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_write_json_report(self):
        """Test writing JSON report."""
        output_path = os.path.join(self.temp_dir, 'test.json')
        data = {'key': 'value', 'number': 42}
        write_json_report(data, output_path)
        self.assertTrue(os.path.exists(output_path))

    def test_write_csv_report(self):
        """Test writing CSV report."""
        output_path = os.path.join(self.temp_dir, 'test.csv')
        rows = [{'name': 'Alice', 'score': 100}, {'name': 'Bob', 'score': 95}]
        write_csv_report(rows, output_path)
        self.assertTrue(os.path.exists(output_path))

    def test_append_csv_report(self):
        """Test appending to CSV report."""
        output_path = os.path.join(self.temp_dir, 'test.csv')
        append_csv_report({'name': 'Alice', 'score': 100}, output_path)
        append_csv_report({'name': 'Bob', 'score': 95}, output_path)
        self.assertTrue(os.path.exists(output_path))


class TestSweepConfig(unittest.TestCase):
    """Test cases for parameter sweep functionality."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.config_file = os.path.join(self.temp_dir, 'test_sweep.yaml')
        with open(self.config_file, 'w') as f:
            f.write("""
scenarios:
  - id: "base_scenario"
    name: "Base Scenario"
    num_robots: 3
    map: "test_map.sdf"
    timeout_sec: 10
    parameters:
      max_linear_velocity: 0.5

sweep:
  - parameter: max_linear_velocity
    values: [0.3, 0.5, 0.8]
    base_scenario: base_scenario
  - parameter: num_robots
    values: [2, 4]
    base_scenario: base_scenario
""")

    def tearDown(self):
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_load_sweep_config(self):
        """Test loading and generating sweep permutations."""
        runner = BenchmarkRunner(self.config_file, self.temp_dir)
        runner.load_scenarios()
        permutations = runner.load_sweep_config()
        self.assertEqual(len(permutations), 5)

    def test_sweep_permutation_values(self):
        """Test that sweep permutations have correct parameter values."""
        runner = BenchmarkRunner(self.config_file, self.temp_dir)
        runner.load_scenarios()
        permutations = runner.load_sweep_config()

        for p in permutations:
            if 'max_linear_velocity' in p['id']:
                self.assertIn('max_linear_velocity', p.get('parameters', {}))
            elif 'num_robots' in p['id']:
                expected = p['parameters'].get('num_robots', p['num_robots'])
                self.assertEqual(p['num_robots'], expected)

    def test_sweep_scenario_ids(self):
        """Test sweep generates unique scenario IDs."""
        runner = BenchmarkRunner(self.config_file, self.temp_dir)
        runner.load_scenarios()
        permutations = runner.load_sweep_config()
        ids = [p['id'] for p in permutations]
        self.assertEqual(len(ids), len(set(ids)))


class TestSystemMetrics(unittest.TestCase):
    """Test cases for SystemMetricsCollector."""

    def test_collector_start_stop(self):
        """Test starting and stopping the collector."""
        collector = SystemMetricsCollector(interval=0.1)
        collector.start()
        import time
        time.sleep(0.3)
        collector.stop()
        self.assertGreater(len(collector.samples), 0)

    def test_get_average_cpu(self):
        """Test average CPU calculation."""
        collector = SystemMetricsCollector()
        collector.samples = [
            {'cpu_percent': 10.0, 'memory_percent': 50.0,
             'memory_used_mb': 1000.0, 'timestamp': 0},
            {'cpu_percent': 20.0, 'memory_percent': 60.0,
             'memory_used_mb': 2000.0, 'timestamp': 1},
        ]
        self.assertEqual(collector.get_average_cpu(), 15.0)

    def test_get_peak_cpu(self):
        """Test peak CPU calculation."""
        collector = SystemMetricsCollector()
        collector.samples = [
            {'cpu_percent': 10.0, 'memory_percent': 50.0,
             'memory_used_mb': 1000.0, 'timestamp': 0},
            {'cpu_percent': 20.0, 'memory_percent': 60.0,
             'memory_used_mb': 2000.0, 'timestamp': 1},
        ]
        self.assertEqual(collector.get_peak_cpu(), 20.0)


if __name__ == '__main__':
    unittest.main()

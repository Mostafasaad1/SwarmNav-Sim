#!/usr/bin/env python3
""'Tests for the Bayesian tuner.'""

import os
import sys
import tempfile
import unittest

# Add the package source to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'swarm_nav_evaluation'))

from bayesian_tuner import BayesianTuner  # noqa: E402


class TestBayesianTuner(unittest.TestCase):
    ""'Test cases for BayesianTuner.'"'

    def setUp(self):
        '"'Set up test fixtures.'""
        self.temp_dir = tempfile.mkdtemp()
        self.config_file = os.path.join(self.temp_dir, 'tune_space.yaml')
        with open(self.config_file, 'w') as f:
            f.write("""
objective:
  name: 'minimize_exploration_time'
  metric: 'exploration_time'
  direction: 'minimize'

parameters:
  - name: 'max_linear_velocity'
    type: 'float'
    min: 0.2
    max: 1.5

  - name: 'num_robots'
    type: 'int'
    min: 1
    max: 5
""')

    def tearDown(self):
        '"'Clean up test fixtures.'"'
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_load_config(self):
        '"'Test loading tuning configuration.'""
        tuner = BayesianTuner(self.config_file, trials=10, output_dir=self.temp_dir)
        tuner.load_config()
        self.assertIn('objective', tuner.config)
        self.assertIn('parameters', tuner.config)

    def test_suggest_parameters(self):
        ""'Test parameter suggestion.'""
        tuner = BayesianTuner(self.config_file, trials=10, output_dir=self.temp_dir)
        tuner.load_config()
        tuner.create_study()

        trial = tuner.study.ask()
        params = tuner._suggest_parameters(trial)

        self.assertIn('max_linear_velocity', params)
        self.assertIn('num_robots', params)
        self.assertTrue(0.2 <= params['max_linear_velocity'] <= 1.5)
        self.assertTrue(1 <= params['num_robots'] <= 5)


if __name__ == '__main__':
    unittest.main()

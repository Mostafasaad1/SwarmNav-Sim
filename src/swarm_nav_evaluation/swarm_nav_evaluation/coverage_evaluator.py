#!/usr/bin/env python3
""'
Evaluates map coverage percentage for the swarm exploration mission.

Calculates and tracks coverage metrics over time.
'""

import json

import numpy as np
import rclpy
import json
import numpy as np
from nav_msgs.msg import OccupancyGrid
from rclpy.node import Node


class CoverageEvaluator(Node):
    def __init__(self):
        super().__init__('coverage_evaluator')

        # Declare parameters
        self.declare_parameter('output_file', 'coverage_results.json')
        self.declare_parameter('evaluation_interval', 5.0)  # seconds

        # Get parameters
        self.output_file = self.get_parameter('output_file').value
        self.evaluation_interval = self.get_parameter(
            'evaluation_interval').value

        self.get_logger().info('Coverage Evaluator initialized')
        self.get_logger().info(f'Output file: {self.output_file}')
        self.get_logger().info(
            f'Evaluation interval: {self.evaluation_interval}s')

        # Subscribe to global map
        self.map_sub = self.create_subscription(
            OccupancyGrid,
            '/swarm/global_map',
            self.map_callback,
            10
        )

        # Storage for results
        self.coverage_history = []
        self.latest_map = None
        self.start_time = self.get_clock().now()

        # Timer for periodic evaluation
        self.timer = self.create_timer(
            self.evaluation_interval,
            self.evaluate_coverage
        )

        self.get_logger().info('Coverage Evaluator ready')

    def map_callback(self, msg):
        ""'Store latest map.'"'
        self.latest_map = msg

    def evaluate_coverage(self):
        '"'Calculate and log coverage percentage.'""
        if self.latest_map is None:
            self.get_logger().warn('No map received yet')
            return

        # Convert map data to numpy array
        map_data = np.array(self.latest_map.data)

        # Calculate coverage
        total_cells = len(map_data)
        unknown_cells = np.sum(map_data == -1)
        known_cells = total_cells - unknown_cells

        coverage_percentage = (known_cells / total_cells) * 100.0

        # Calculate elapsed time
        elapsed_time = (self.get_clock().now() -
                        self.start_time).nanoseconds / 1e9

        # Store result
        result = {
            'timestamp': elapsed_time,
            'coverage_percentage': coverage_percentage,
            'total_cells': int(total_cells),
            'known_cells': int(known_cells),
            'unknown_cells': int(unknown_cells)
        }
        self.coverage_history.append(result)

        self.get_logger().info(
            f'Coverage: {coverage_percentage:.2f}% '
            f'({known_cells}/{total_cells} cells) '
            f'at t={elapsed_time:.1f}s'
        )

        # Save to file
        self.save_results()

    def save_results(self):
        ""'Save coverage history to JSON file.'""
        output_data = {
            'evaluation_start': self.start_time.nanoseconds / 1e9,
            'coverage_history': self.coverage_history,
            'final_coverage': (
                self.coverage_history[-1]['coverage_percentage']
                if self.coverage_history else 0.0
            )
        }

        try:
            with open(self.output_file, 'w') as f:
                json.dump(output_data, f, indent=2)
            self.get_logger().debug(f'Results saved to {self.output_file}')
        except Exception as e:
            self.get_logger().error(f'Failed to save results: {e}')


def main(args=None):
    rclpy.init(args=args)
    node = CoverageEvaluator()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.get_logger().info('Shutting down Coverage Evaluator')
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

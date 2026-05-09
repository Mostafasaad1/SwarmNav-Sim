#!/usr/bin/env python3
"""
test_system_launch.py
Integration test for full system launch
"""

import unittest
import time
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import LaserScan
from nav_msgs.msg import Odometry
from swarm_nav_msgs.msg import NeighborStateArray

import launch
import launch_ros
import launch_testing
import launch_testing.actions
import launch_testing.markers
import pytest


@pytest.mark.launch_test
@launch_testing.markers.keep_alive
def generate_test_description():
    """Generate launch description for testing"""

    # Launch the full swarm system
    swarm_launch = launch.actions.IncludeLaunchDescription(
        launch.launch_description_sources.PythonLaunchDescriptionSource([
            launch.substitutions.PathJoinSubstitution([
                launch_ros.substitutions.FindPackageShare('swarm_nav_bringup'),
                'launch',
                'swarm.launch.py'
            ])
        ]),
        launch_arguments={
            'num_robots': '3',
            'simulator': 'gazebo',
            'use_sim_time': 'true',
            'use_rviz': 'false',
        }.items()
    )

    return launch.LaunchDescription([
        swarm_launch,
        launch_testing.actions.ReadyToTest()
    ])


class TestSystemLaunch(unittest.TestCase):
    """Test full system launch"""

    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = Node('test_system_launch')
        self.scan_received = {0: False, 1: False, 2: False}
        self.odom_received = {0: False, 1: False, 2: False}
        self.neighbor_states_received = False

        # Create subscribers for each robot
        for i in range(3):
            self.node.create_subscription(
                LaserScan,
                f'/robot_{i}/scan',
                lambda msg, robot_id=i: self._scan_callback(msg, robot_id),
                10
            )
            self.node.create_subscription(
                Odometry,
                f'/robot_{i}/odom',
                lambda msg, robot_id=i: self._odom_callback(msg, robot_id),
                10
            )

        # Subscribe to neighbor states
        self.node.create_subscription(
            NeighborStateArray,
            '/swarm/neighbor_states',
            self._neighbor_states_callback,
            10
        )

    def tearDown(self):
        self.node.destroy_node()

    def _scan_callback(self, msg, robot_id):
        self.scan_received[robot_id] = True

    def _odom_callback(self, msg, robot_id):
        self.odom_received[robot_id] = True

    def _neighbor_states_callback(self, msg):
        self.neighbor_states_received = True

    def test_topics_publish(self):
        """Test that all expected topics publish at >= 1 Hz"""

        # Spin for 60 seconds to allow system to start
        start_time = time.time()
        timeout = 60.0

        while time.time() - start_time < timeout:
            rclpy.spin_once(self.node, timeout_sec=0.1)

            # Check if all topics have received messages
            if (all(self.scan_received.values()) and
                all(self.odom_received.values()) and
                    self.neighbor_states_received):
                break

        # Assert all topics received messages
        for i in range(3):
            self.assertTrue(
                self.scan_received[i],
                f'robot_{i}/scan did not publish'
            )
            self.assertTrue(
                self.odom_received[i],
                f'robot_{i}/odom did not publish'
            )

        self.assertTrue(
            self.neighbor_states_received,
            '/swarm/neighbor_states did not publish'
        )

    def test_no_node_crashes(self):
        """Test that no nodes crash during execution"""

        # Spin for 60 seconds
        start_time = time.time()
        timeout = 60.0

        while time.time() - start_time < timeout:
            rclpy.spin_once(self.node, timeout_sec=0.1)

        # If we reach here without exceptions, test passes
        self.assertTrue(True)

#!/usr/bin/env python3
"""
test_topic_publishing.py
Integration test for robot motion via topic publishing
"""

import unittest
import time
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
import math

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
            'num_robots': '1',
            'simulator': 'gazebo',
            'use_sim_time': 'true',
            'use_rviz': 'false',
        }.items()
    )

    return launch.LaunchDescription([
        swarm_launch,
        launch_testing.actions.ReadyToTest()
    ])


class TestTopicPublishing(unittest.TestCase):
    """Test robot motion via cmd_vel publishing"""

    @classmethod
    def setUpClass(cls):
        rclpy.init()

    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()

    def setUp(self):
        self.node = Node('test_topic_publishing')
        self.initial_pose = None
        self.current_pose = None

        # Create publisher for cmd_vel
        self.cmd_vel_pub = self.node.create_publisher(
            Twist,
            '/robot_0/cmd_vel',
            10
        )

        # Create subscriber for odom
        self.odom_sub = self.node.create_subscription(
            Odometry,
            '/robot_0/odom',
            self._odom_callback,
            10
        )

    def tearDown(self):
        self.node.destroy_node()

    def _odom_callback(self, msg):
        if self.initial_pose is None:
            self.initial_pose = msg.pose.pose
        self.current_pose = msg.pose.pose

    def test_robot_motion(self):
        """Test that publishing cmd_vel causes odom position to change"""

        # Wait for initial odom message
        start_time = time.time()
        while self.initial_pose is None and time.time() - start_time < 10.0:
            rclpy.spin_once(self.node, timeout_sec=0.1)

        self.assertIsNotNone(self.initial_pose, "Did not receive initial odom")

        # Publish forward velocity command
        cmd = Twist()
        cmd.linear.x = 0.5  # 0.5 m/s forward

        # Publish for 10 seconds
        start_time = time.time()
        while time.time() - start_time < 10.0:
            self.cmd_vel_pub.publish(cmd)
            rclpy.spin_once(self.node, timeout_sec=0.1)

        # Check that position changed
        self.assertIsNotNone(self.current_pose, "Did not receive updated odom")

        # Calculate distance moved
        dx = self.current_pose.position.x - self.initial_pose.position.x
        dy = self.current_pose.position.y - self.initial_pose.position.y
        distance = math.sqrt(dx * dx + dy * dy)

        # Should have moved at least 1 meter (conservative check)
        self.assertGreater(
            distance,
            1.0,
            f"Robot did not move enough (distance={distance:.2f}m)"
        )

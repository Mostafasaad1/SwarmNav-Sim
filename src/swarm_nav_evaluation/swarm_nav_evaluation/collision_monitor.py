#!/usr/bin/env python3
"""
Monitors for collisions between robots and obstacles.

Tracks robot positions and detects collision events.
"""

import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node


class CollisionMonitor(Node):
    def __init__(self):
        super().__init__('collision_monitor')

        # Declare parameters
        self.declare_parameter('num_robots', 3)
        self.declare_parameter('collision_threshold', 0.5)  # meters
        self.declare_parameter('output_file', 'collision_results.json')

        # Get parameters
        self.num_robots = self.get_parameter('num_robots').value
        self.collision_threshold = self.get_parameter(
            'collision_threshold').value
        self.output_file = self.get_parameter('output_file').value

        self.get_logger().info('Collision Monitor initialized')
        self.get_logger().info(f'Monitoring {self.num_robots} robots')
        self.get_logger().info(
            f'Collision threshold: {self.collision_threshold}m')

        # Storage for robot poses
        self.robot_poses = {}

        # Subscribe to each robot's odometry
        self.odom_subs = []
        for i in range(self.num_robots):
            robot_id = f'robot_{i}'
            sub = self.create_subscription(
                Odometry,
                f'/{robot_id}/odom',
                lambda msg, rid=robot_id: self.odom_callback(msg, rid),
                10
            )
            self.odom_subs.append(sub)

        # Collision tracking
        self.collision_count = 0
        self.collision_events = []
        self.start_time = self.get_clock().now()

        # Timer for periodic collision checking
        self.timer = self.create_timer(0.1, self.check_collisions)

        self.get_logger().info('Collision Monitor ready')

    def odom_callback(self, msg, robot_id):
        """Store latest robot pose."""
        self.robot_poses[robot_id] = msg.pose.pose

    def check_collisions(self):
        """Check for collisions between all robot pairs."""
        if len(self.robot_poses) < 2:
            return

        robot_ids = list(self.robot_poses.keys())

        for i in range(len(robot_ids)):
            for j in range(i + 1, len(robot_ids)):
                robot_a = robot_ids[i]
                robot_b = robot_ids[j]

                pose_a = self.robot_poses[robot_a]
                pose_b = self.robot_poses[robot_b]

                # Calculate distance
                dx = pose_a.position.x - pose_b.position.x
                dy = pose_a.position.y - pose_b.position.y
                distance = np.sqrt(dx**2 + dy**2)

                # Check for collision
                if distance < self.collision_threshold:
                    self.record_collision(robot_a, robot_b, distance)

    def record_collision(self, robot_a, robot_b, distance):
        """Record a collision event."""
        elapsed_time = (self.get_clock().now() -
                        self.start_time).nanoseconds / 1e9

        collision_event = {
            'timestamp': elapsed_time,
            'robot_a': robot_a,
            'robot_b': robot_b,
            'distance': float(distance)
        }

        self.collision_events.append(collision_event)
        self.collision_count += 1

        self.get_logger().warn(
            f'COLLISION #{self.collision_count}: {robot_a} <-> {robot_b} '
            f'(distance: {distance:.3f}m) at t={elapsed_time:.1f}s'
        )

        # Save results
        self.save_results()

    def save_results(self):
        """Save collision data to JSON file."""
        output_data = {
            'evaluation_start': self.start_time.nanoseconds / 1e9,
            'num_robots': self.num_robots,
            'collision_threshold': self.collision_threshold,
            'total_collisions': self.collision_count,
            'collision_events': self.collision_events
        }

        try:
            with open(self.output_file, 'w') as f:
                json.dump(output_data, f, indent=2)
            self.get_logger().debug(f'Results saved to {self.output_file}')
        except Exception as e:
            self.get_logger().error(f'Failed to save results: {e}')


def main(args=None):
    rclpy.init(args=args)
    node = CollisionMonitor()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.get_logger().info(
            f'Total collisions detected: {node.collision_count}')
        node.get_logger().info('Shutting down Collision Monitor')
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

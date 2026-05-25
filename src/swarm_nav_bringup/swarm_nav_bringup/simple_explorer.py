#!/usr/bin/env python3
"""
Simple random exploration node for testing.

Makes robots move randomly to explore the environment.
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from sensor_msgs.msg import LaserScan
import random
import math


class SimpleExplorer(Node):
    """Simple random exploration behavior."""

    def __init__(self):
        super().__init__('simple_explorer')
        
        # Declare parameters
        self.declare_parameter('robot_id', 'robot_0')
        self.declare_parameter('linear_speed', 0.3)
        self.declare_parameter('angular_speed', 0.5)
        self.declare_parameter('obstacle_distance', 0.5)
        
        # Get parameters
        self.robot_id = self.get_parameter('robot_id').value
        self.linear_speed = self.get_parameter('linear_speed').value
        self.angular_speed = self.get_parameter('angular_speed').value
        self.obstacle_distance = self.get_parameter('obstacle_distance').value
        
        # Publishers and subscribers
        self.cmd_vel_pub = self.create_publisher(
            Twist,
            f'/{self.robot_id}/cmd_vel',
            10
        )
        
        self.scan_sub = self.create_subscription(
            LaserScan,
            f'/{self.robot_id}/scan',
            self.scan_callback,
            10
        )
        
        # State
        self.obstacle_detected = False
        self.min_distance = float('inf')
        self.last_scan_time = self.get_clock().now()
        
        # Control timer
        self.timer = self.create_timer(0.1, self.control_loop)
        
        # Behavior state
        self.state = 'forward'  # 'forward', 'turning'
        self.turn_duration = 0
        self.turn_counter = 0
        
        self.get_logger().info(f'Simple explorer started for {self.robot_id}')

    def scan_callback(self, msg: LaserScan):
        """Process laser scan data."""
        self.last_scan_time = self.get_clock().now()
        
        # Check for obstacles in front (center 60 degrees)
        num_readings = len(msg.ranges)
        center_start = num_readings // 3
        center_end = 2 * num_readings // 3
        
        front_ranges = [
            r for r in msg.ranges[center_start:center_end]
            if not math.isinf(r) and not math.isnan(r)
        ]
        
        if front_ranges:
            self.min_distance = min(front_ranges)
            self.obstacle_detected = self.min_distance < self.obstacle_distance
        else:
            self.min_distance = float('inf')
            self.obstacle_detected = False

    def control_loop(self):
        """Main control loop."""
        cmd = Twist()
        
        # Check if we have recent scan data
        time_since_scan = (self.get_clock().now() - self.last_scan_time).nanoseconds / 1e9
        if time_since_scan > 1.0:
            # No recent scan data, stop
            self.cmd_vel_pub.publish(cmd)
            return
        
        if self.state == 'forward':
            if self.obstacle_detected:
                # Switch to turning
                self.state = 'turning'
                self.turn_duration = random.randint(10, 30)  # 1-3 seconds
                self.turn_counter = 0
                self.get_logger().info(
                    f'{self.robot_id}: Obstacle at {self.min_distance:.2f}m, turning'
                )
            else:
                # Move forward
                cmd.linear.x = self.linear_speed
                cmd.angular.z = random.uniform(-0.1, 0.1)  # Small random turns
        
        elif self.state == 'turning':
            # Turn in place
            cmd.angular.z = self.angular_speed if random.random() > 0.5 else -self.angular_speed
            self.turn_counter += 1
            
            if self.turn_counter >= self.turn_duration:
                # Done turning, go forward
                self.state = 'forward'
                self.get_logger().info(f'{self.robot_id}: Done turning, moving forward')
        
        # Publish command
        self.cmd_vel_pub.publish(cmd)


def main(args=None):
    rclpy.init(args=args)
    node = SimpleExplorer()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

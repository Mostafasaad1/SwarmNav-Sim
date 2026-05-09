#!/usr/bin/env python3
"""
timer_shutdown.py
Node that shuts down the ROS system after a configurable duration
Used for automated evaluation runs with fixed time limits
"""

import rclpy
from rclpy.node import Node


class TimerShutdownNode(Node):
    """Node that triggers system shutdown after a specified duration"""
    
    def __init__(self):
        super().__init__('timer_shutdown')
        
        # Declare parameters
        self.declare_parameter('duration', 300)  # Default: 5 minutes
        
        # Get parameters
        duration = self.get_parameter('duration').value
        
        self.get_logger().info(f'Timer shutdown node started. Will shutdown in {duration} seconds.')
        
        # Create timer for shutdown
        self.shutdown_timer = self.create_timer(
            duration,
            self.shutdown_callback
        )
    
    def shutdown_callback(self):
        """Callback to shutdown the system"""
        self.get_logger().info('Evaluation duration reached. Shutting down...')
        
        # Cancel timer
        self.shutdown_timer.cancel()
        
        # Trigger shutdown
        rclpy.shutdown()


def main(args=None):
    rclpy.init(args=args)
    
    node = TimerShutdownNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()

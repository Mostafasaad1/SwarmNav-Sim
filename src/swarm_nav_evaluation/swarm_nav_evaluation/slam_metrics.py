#!/usr/bin/env python3
"""
Evaluates SLAM performance metrics.

Calculates Absolute Trajectory Error (ATE) for SLAM evaluation.
"""

import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped
from rclpy.node import Node
from tf2_ros import Buffer, TransformListener
from tf2_ros import LookupException, ConnectivityException, ExtrapolationException


class SlamMetrics(Node):
    def __init__(self):
        super().__init__('slam_metrics')

        self.declare_parameter('num_robots', 3)
        self.declare_parameter('output_file', 'slam_metrics.json')
        self.declare_parameter('evaluation_interval', 5.0)
        self.declare_parameter('slam_pose_source', 'topic')
        self.declare_parameter('ground_truth_source', 'topic')
        self.declare_parameter('base_frame', 'base_footprint')
        self.declare_parameter('map_frame', 'map')
        self.declare_parameter('world_frame', 'world')

        self.num_robots = self.get_parameter('num_robots').value
        self.output_file = self.get_parameter('output_file').value
        self.evaluation_interval = self.get_parameter(
            'evaluation_interval').value
        self.slam_pose_source = self.get_parameter('slam_pose_source').value
        self.ground_truth_source = self.get_parameter('ground_truth_source').value
        self.base_frame = self.get_parameter('base_frame').value
        self.map_frame = self.get_parameter('map_frame').value
        self.world_frame = self.get_parameter('world_frame').value

        self.get_logger().info('SLAM Metrics Evaluator initialized')
        self.get_logger().info(f'Monitoring {self.num_robots} robots')
        self.get_logger().info(f'SLAM pose source: {self.slam_pose_source}')
        self.get_logger().info(f'Ground truth source: {self.ground_truth_source}')

        self.slam_trajectories = {}
        self.ground_truth_trajectories = {}

        self.slam_subs = []
        self.gt_subs = []

        self.tf_buffer = None
        self.tf_listener = None

        if self.slam_pose_source == 'tf' or self.ground_truth_source == 'tf':
            self.tf_buffer = Buffer()
            self.tf_listener = TransformListener(self.tf_buffer, self)

        for i in range(self.num_robots):
            robot_id = f'robot_{i}'

            if self.slam_pose_source == 'topic':
                slam_sub = self.create_subscription(
                    Odometry,
                    f'/{robot_id}/odom',
                    lambda msg, rid=robot_id: self.slam_callback(msg, rid),
                    10
                )
                self.slam_subs.append(slam_sub)
            elif self.slam_pose_source == 'pose':
                slam_sub = self.create_subscription(
                    PoseStamped,
                    f'/{robot_id}/slam_pose',
                    lambda msg, rid=robot_id: self.pose_to_slam_callback(msg, rid),
                    10
                )
                self.slam_subs.append(slam_sub)

            if self.ground_truth_source == 'topic':
                gt_sub = self.create_subscription(
                    Odometry,
                    f'/{robot_id}/ground_truth/odom',
                    lambda msg, rid=robot_id: self.ground_truth_callback(msg, rid),
                    10
                )
                self.gt_subs.append(gt_sub)
            elif self.ground_truth_source == 'pose':
                gt_sub = self.create_subscription(
                    PoseStamped,
                    f'/{robot_id}/ground_truth_pose',
                    lambda msg, rid=robot_id: self.pose_to_ground_truth_callback(msg, rid),
                    10
                )
                self.gt_subs.append(gt_sub)

            self.slam_trajectories[robot_id] = []
            self.ground_truth_trajectories[robot_id] = []

        self.timer = self.create_timer(
            self.evaluation_interval,
            self.evaluate_metrics
        )

        self.start_time = self.get_clock().now()
        self.metrics_history = []

        self.get_logger().info('SLAM Metrics Evaluator ready')

    def get_pose_from_tf(self, target_frame: str, source_frame: str):
        try:
            t = self.tf_buffer.lookup_transform(
                target_frame,
                source_frame,
                rclpy.time.Time()
            )
            return {
                'x': t.transform.translation.x,
                'y': t.transform.translation.y,
                'z': t.transform.translation.z
            }
        except (LookupException, ConnectivityException, ExtrapolationException) as e:
            self.get_logger().debug(f'TF lookup failed: {e}')
            return None

    def capture_tf_poses(self):
        for i in range(self.num_robots):
            robot_id = f'robot_{i}'
            base_frame = f'{robot_id}/{self.base_frame}'

            if self.slam_pose_source == 'tf':
                slam_pose = self.get_pose_from_tf(self.map_frame, base_frame)
                if slam_pose:
                    slam_pose['timestamp'] = self.get_clock().now().nanoseconds / 1e9
                    self.slam_trajectories[robot_id].append(slam_pose)

            if self.ground_truth_source == 'tf':
                gt_pose = self.get_pose_from_tf(self.world_frame, base_frame)
                if gt_pose:
                    gt_pose['timestamp'] = self.get_clock().now().nanoseconds / 1e9
                    self.ground_truth_trajectories[robot_id].append(gt_pose)

    def pose_to_slam_callback(self, msg: PoseStamped, robot_id: str):
        pose = {
            'timestamp': self.get_clock().now().nanoseconds / 1e9,
            'x': msg.pose.position.x,
            'y': msg.pose.position.y,
            'z': msg.pose.position.z
        }
        self.slam_trajectories[robot_id].append(pose)

    def pose_to_ground_truth_callback(self, msg: PoseStamped, robot_id: str):
        pose = {
            'timestamp': self.get_clock().now().nanoseconds / 1e9,
            'x': msg.pose.position.x,
            'y': msg.pose.position.y,
            'z': msg.pose.position.z
        }
        self.ground_truth_trajectories[robot_id].append(pose)

    def slam_callback(self, msg, robot_id):
        """Store SLAM estimated pose."""
        pose = {
            'timestamp': self.get_clock().now().nanoseconds / 1e9,
            'x': msg.pose.pose.position.x,
            'y': msg.pose.pose.position.y,
            'z': msg.pose.pose.position.z
        }
        self.slam_trajectories[robot_id].append(pose)

    def ground_truth_callback(self, msg, robot_id):
        """Store ground truth pose."""
        pose = {
            'timestamp': self.get_clock().now().nanoseconds / 1e9,
            'x': msg.pose.pose.position.x,
            'y': msg.pose.pose.position.y,
            'z': msg.pose.pose.position.z
        }
        self.ground_truth_trajectories[robot_id].append(pose)

    def evaluate_metrics(self):
        """Calculate ATE (Absolute Trajectory Error) for each robot."""

        if self.tf_buffer is not None:
            self.capture_tf_poses()

        elapsed_time = (self.get_clock().now() -
                        self.start_time).nanoseconds / 1e9

        robot_metrics = {}
        total_ate = 0.0
        robot_count = 0

        for robot_id in self.slam_trajectories.keys():
            slam_traj = self.slam_trajectories[robot_id]
            gt_traj = self.ground_truth_trajectories[robot_id]

            if len(slam_traj) < 2 or len(gt_traj) < 2:
                self.get_logger().debug(
                    f'{robot_id}: Not enough data '
                    f'(SLAM: {len(slam_traj)}, GT: {len(gt_traj)})')
                continue

            ate = self.calculate_ate(slam_traj, gt_traj)

            robot_metrics[robot_id] = {
                'ate_rmse': ate,
                'trajectory_length': len(slam_traj)
            }

            total_ate += ate
            robot_count += 1

            self.get_logger().info(
                f'{robot_id}: ATE RMSE = {ate:.4f}m '
                f'({len(slam_traj)} poses)'
            )

        avg_ate = total_ate / robot_count if robot_count > 0 else 0.0

        metrics = {
            'timestamp': elapsed_time,
            'average_ate_rmse': avg_ate,
            'robot_metrics': robot_metrics
        }

        self.metrics_history.append(metrics)

        self.get_logger().info(
            f'Average ATE RMSE: {avg_ate:.4f}m at t={elapsed_time:.1f}s'
        )

        self.save_results()

    def calculate_ate(self, slam_traj, gt_traj):
        """Calculate Absolute Trajectory Error (ATE) RMSE."""

        min_len = min(len(slam_traj), len(gt_traj))
        if min_len == 0:
            return 0.0

        errors = []
        for i in range(min_len):
            slam_pose = slam_traj[i]
            gt_pose = gt_traj[i]

            dx = slam_pose['x'] - gt_pose['x']
            dy = slam_pose['y'] - gt_pose['y']
            dz = slam_pose['z'] - gt_pose['z']

            error = np.sqrt(dx**2 + dy**2 + dz**2)
            errors.append(error)

        rmse = np.sqrt(np.mean(np.array(errors)**2))
        return float(rmse)

    def save_results(self):
        """Save metrics to JSON file."""
        output_data = {
            'evaluation_start': self.start_time.nanoseconds / 1e9,
            'num_robots': self.num_robots,
            'metrics_history': self.metrics_history,
            'final_average_ate': (
                self.metrics_history[-1]['average_ate_rmse']
                if self.metrics_history else 0.0
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
    node = SlamMetrics()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.get_logger().info('Shutting down SLAM Metrics Evaluator')
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

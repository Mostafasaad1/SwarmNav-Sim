#!/usr/bin/env python3
"""
Launch file for evaluation nodes.

Launches coverage, collision, and SLAM metrics evaluation nodes.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """Generate launch description for evaluation nodes."""
    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots in the swarm'
    )

    output_dir_arg = DeclareLaunchArgument(
        'output_dir',
        default_value='./evaluation_results',
        description='Directory to save evaluation results'
    )

    # Get launch configurations
    num_robots = LaunchConfiguration('num_robots')
    output_dir = LaunchConfiguration('output_dir')

    # Create launch description
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(num_robots_arg)
    ld.add_action(output_dir_arg)

    # Coverage evaluator node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='coverage_evaluator.py',
        name='coverage_evaluator',
        parameters=[{
            'use_sim_time': True,
            'output_file': [output_dir, '/coverage_results.json'],
            'evaluation_interval': 5.0
        }],
        output='screen'
    ))

    # Collision monitor node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='collision_monitor.py',
        name='collision_monitor',
        parameters=[{
            'use_sim_time': True,
            'num_robots': num_robots,
            'collision_threshold': 0.5,
            'output_file': [output_dir, '/collision_results.json']
        }],
        output='screen'
    ))

    # SLAM metrics node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='slam_metrics.py',
        name='slam_metrics',
        parameters=[{
            'use_sim_time': True,
            'num_robots': num_robots,
            'output_file': [output_dir, '/slam_metrics.json'],
            'evaluation_interval': 5.0
        }],
        output='screen'
    ))

    return ld


if __name__ == '__main__':
    generate_launch_description()

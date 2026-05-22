#!/usr/bin/env python3
"""
Headless benchmark launch file for automated evaluation runs.

Launches the swarm in headless mode with evaluation nodes and a timer shutdown.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    """Generate launch description for headless benchmark runs."""
    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots in the swarm'
    )

    duration_arg = DeclareLaunchArgument(
        'duration',
        default_value='300',
        description='Duration before shutting down the evaluation in seconds'
    )

    output_dir_arg = DeclareLaunchArgument(
        'output_dir',
        default_value='./evaluation_results',
        description='Directory to save evaluation results'
    )

    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='false',
        description='Launch simulator with GUI (default: false for headless)'
    )

    max_linear_velocity_arg = DeclareLaunchArgument(
        'max_linear_velocity',
        default_value='0.5',
        description='Maximum linear velocity override'
    )

    max_angular_velocity_arg = DeclareLaunchArgument(
        'max_angular_velocity',
        default_value='1.0',
        description='Maximum angular velocity override'
    )

    robot_radius_arg = DeclareLaunchArgument(
        'robot_radius',
        default_value='0.25',
        description='Robot radius override'
    )

    time_horizon_arg = DeclareLaunchArgument(
        'time_horizon',
        default_value='2.0',
        description='ORCA time horizon override'
    )

    # Get launch configurations
    num_robots = LaunchConfiguration('num_robots')
    duration = LaunchConfiguration('duration')
    output_dir = LaunchConfiguration('output_dir')

    swarm_nav_bringup_share = FindPackageShare('swarm_nav_bringup')

    # Create launch description
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(num_robots_arg)
    ld.add_action(duration_arg)
    ld.add_action(output_dir_arg)
    ld.add_action(gui_arg)
    ld.add_action(max_linear_velocity_arg)
    ld.add_action(max_angular_velocity_arg)
    ld.add_action(robot_radius_arg)
    ld.add_action(time_horizon_arg)

    # Swarm Launch (headless by default via gui_arg)
    swarm_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([swarm_nav_bringup_share, 'launch', 'swarm.launch.py'])
        ),
        launch_arguments={
            'num_robots': num_robots,
            'use_rviz': LaunchConfiguration('gui'),
            'gui': LaunchConfiguration('gui'),
            'simulator': 'gazebo',
            'max_linear_velocity': LaunchConfiguration('max_linear_velocity'),
            'max_angular_velocity': LaunchConfiguration('max_angular_velocity'),
            'robot_radius': LaunchConfiguration('robot_radius'),
            'time_horizon': LaunchConfiguration('time_horizon'),
        }.items()
    )
    ld.add_action(swarm_launch)

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

    # System metrics node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='system_metrics.py',
        name='system_metrics',
        parameters=[{
            'use_sim_time': True,
            'interval': 1.0
        }],
        output='screen'
    ))

    # Timer Shutdown Node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='timer_shutdown.py',
        name='timer_shutdown',
        parameters=[{
            'use_sim_time': True,
            'duration': duration
        }],
        output='screen'
    ))

    return ld


if __name__ == '__main__':
    generate_launch_description()

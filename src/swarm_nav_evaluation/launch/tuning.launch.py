#!/usr/bin/env python3
"""
Launch wrapper for the Bayesian tuner node.

Launches the bayesian_tuner.py node with parameters from tune_space.yaml.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """Generate launch description for the Bayesian tuner."""
    # Launch arguments
    tune_config_arg = DeclareLaunchArgument(
        'tune_config',
        default_value='',
        description='Path to the tune_space.yaml configuration file'
    )

    trials_arg = DeclareLaunchArgument(
        'trials',
        default_value='50',
        description='Number of Optuna trials to run'
    )

    output_dir_arg = DeclareLaunchArgument(
        'output_dir',
        default_value='~/.ros/swarm_nav_results',
        description='Directory to write tuning results and plots'
    )

    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='false',
        description='Launch simulator with GUI'
    )

    # Get launch configurations
    tune_config = LaunchConfiguration('tune_config')
    trials = LaunchConfiguration('trials')
    output_dir = LaunchConfiguration('output_dir')
    gui = LaunchConfiguration('gui')

    # Create launch description
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(tune_config_arg)
    ld.add_action(trials_arg)
    ld.add_action(output_dir_arg)
    ld.add_action(gui_arg)

    # Bayesian Tuner Node
    ld.add_action(Node(
        package='swarm_nav_evaluation',
        executable='bayesian_tuner.py',
        name='bayesian_tuner',
        parameters=[{
            'tune_config': tune_config,
            'trials': trials,
            'output_dir': output_dir,
            'gui': gui,
        }],
        output='screen'
    ))

    return ld


if __name__ == '__main__':
    generate_launch_description()

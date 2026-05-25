#!/usr/bin/env python3
"""
Minimal launch file for SwarmNav-Sim - Gazebo Ignition only.

Launches only Gazebo Ignition Fortress without robot nodes.
Use this while C++ packages are being fixed.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    """Generate minimal launch description with only Gazebo."""
    # Get package directory
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')

    # World file path
    world_file = os.path.join(pkg_swarm_nav_bringup, 'worlds', 'warehouse.world')

    # Launch arguments
    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='true',
        description='Launch Gazebo GUI'
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation time'
    )

    # Launch Gazebo Ignition Fortress with world
    # -r flag starts simulation running
    gz_sim = ExecuteProcess(
        cmd=['gz', 'sim', world_file, '-r'],
        output='screen'
    )

    return LaunchDescription([
        gui_arg,
        use_sim_time_arg,
        gz_sim
    ])


if __name__ == '__main__':
    generate_launch_description()

#!/usr/bin/env python3
"""
Simplified Gazebo Fortress launch - just simulator.

Launches only Gazebo Ignition Fortress with the warehouse world.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    # Get package directory
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')

    # World file path (Gazebo Ignition SDF format)
    world_file = os.path.join(pkg_swarm_nav_bringup, 'worlds', 'warehouse_gz.sdf')

    # Launch arguments
    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='true',
        description='Launch Gazebo GUI'
    )

    # Launch Gazebo Ignition Fortress with world
    gz_sim = ExecuteProcess(
        cmd=['gz', 'sim', world_file, '-r'],
        output='screen'
    )

    return LaunchDescription([
        gui_arg,
        gz_sim
    ])


if __name__ == '__main__':
    generate_launch_description()

#!/usr/bin/env python3
"""
Ignition Gazebo (Fortress) launch file for SwarmNav-Sim.

Launches Ignition Gazebo using 'ign gazebo' command.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    # Get package directory
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')

    # World file path (use the Gazebo Ignition SDF format)
    world_file = os.path.join(pkg_swarm_nav_bringup, 'worlds', 'warehouse_gz.sdf')

    # Launch arguments
    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='true',
        description='Launch Gazebo GUI'
    )

    verbose_arg = DeclareLaunchArgument(
        'verbose',
        default_value='false',
        description='Verbose output'
    )

    # Build ign gazebo command
    ign_args = [world_file, '-r']  # -r for run (start paused=false)
    
    # Launch Ignition Gazebo
    ign_gazebo = ExecuteProcess(
        cmd=['ign', 'gazebo'] + ign_args,
        output='screen',
        shell=False
    )

    return LaunchDescription([
        gui_arg,
        verbose_arg,
        ign_gazebo
    ])


if __name__ == '__main__':
    generate_launch_description()

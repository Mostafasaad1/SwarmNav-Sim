#!/usr/bin/env python3
"""
Gazebo Classic launch file for SwarmNav-Sim.

Launches Gazebo Classic with warehouse world.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
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

    verbose_arg = DeclareLaunchArgument(
        'verbose',
        default_value='false',
        description='Verbose output'
    )

    # Launch Gazebo Classic server with world
    gzserver = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('gazebo_ros'),
            '/launch/gzserver.launch.py'
        ]),
        launch_arguments={
            'world': world_file,
            'verbose': LaunchConfiguration('verbose')
        }.items()
    )

    # Launch Gazebo Classic client (GUI)
    gzclient = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            FindPackageShare('gazebo_ros'),
            '/launch/gzclient.launch.py'
        ]),
        condition=IfCondition(LaunchConfiguration('gui'))
    )

    return LaunchDescription([
        gui_arg,
        verbose_arg,
        gzserver,
        gzclient
    ])


if __name__ == '__main__':
    generate_launch_description()

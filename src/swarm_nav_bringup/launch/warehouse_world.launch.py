#!/usr/bin/env python3
"""
warehouse_world.launch.py
Launch file for warehouse world with configurable number of robots
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, TextSubstitution
from launch_ros.actions import Node, PushRosNamespace
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    """Generate launch description for warehouse world"""
    
    # Package directories
    bringup_dir = get_package_share_directory('swarm_nav_bringup')
    
    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots to spawn (3-5)'
    )
    
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation time'
    )
    
    world_file_arg = DeclareLaunchArgument(
        'world_file',
        default_value=os.path.join(bringup_dir, 'worlds', 'warehouse.world'),
        description='Path to warehouse world file'
    )
    
    # Launch configuration
    num_robots = LaunchConfiguration('num_robots')
    use_sim_time = LaunchConfiguration('use_sim_time')
    world_file = LaunchConfiguration('world_file')
    
    # Robot state publisher for URDF
    urdf_file = os.path.join(bringup_dir, 'urdf', 'swarm_robot.urdf.xacro')
    
    # Create launch description
    ld = LaunchDescription()
    
    # Add launch arguments
    ld.add_action(num_robots_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(world_file_arg)
    
    # Note: Actual simulator launch (Isaac Sim or Gazebo) would be added here
    # For now, this is a placeholder structure
    
    return ld


if __name__ == '__main__':
    generate_launch_description()

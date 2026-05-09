#!/usr/bin/env python3
"""
Ignition Gazebo with robot spawning.

Launches Ignition Gazebo and spawns robots without navigation nodes.
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def spawn_robots(context, *args, **kwargs):
    """Spawn robots based on num_robots parameter."""
    num_robots = int(LaunchConfiguration('num_robots').perform(context))
    
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')
    urdf_file = os.path.join(pkg_swarm_nav_bringup, 'urdf', 'swarm_robot.urdf.xacro')
    
    # Starting positions for robots (spread them out)
    positions = [
        (-5.0, -5.0, 0.1),
        (5.0, -5.0, 0.1),
        (0.0, 5.0, 0.1),
        (-5.0, 5.0, 0.1),
        (5.0, 5.0, 0.1),
    ]
    
    spawn_actions = []
    
    for i in range(min(num_robots, 5)):
        robot_name = f'robot_{i}'
        x, y, z = positions[i]
        
        # Process xacro to get URDF
        robot_desc = xacro.process_file(urdf_file, mappings={'robot_name': robot_name}).toxml()
        
        # Spawn robot in Ignition
        spawn_robot = Node(
            package='ros_gz_sim',
            executable='create',
            arguments=[
                '-name', robot_name,
                '-x', str(x),
                '-y', str(y),
                '-z', str(z),
                '-topic', f'/robot_{i}/robot_description'
            ],
            output='screen'
        )
        
        # Robot state publisher
        robot_state_publisher = Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name=f'robot_state_publisher_{i}',
            namespace=robot_name,
            parameters=[{'robot_description': robot_desc}],
            output='screen'
        )
        
        spawn_actions.extend([spawn_robot, robot_state_publisher])
    
    return spawn_actions


def generate_launch_description():
    # Get package directory
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')

    # World file path
    world_file = os.path.join(pkg_swarm_nav_bringup, 'worlds', 'warehouse_gz.sdf')

    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots to spawn'
    )

    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='true',
        description='Launch Gazebo GUI'
    )

    # Launch Ignition Gazebo
    ign_gazebo = ExecuteProcess(
        cmd=['ign', 'gazebo', world_file, '-r'],
        output='screen'
    )

    # Spawn robots using OpaqueFunction
    spawn_robots_action = OpaqueFunction(function=spawn_robots)

    return LaunchDescription([
        num_robots_arg,
        gui_arg,
        ign_gazebo,
        spawn_robots_action
    ])


if __name__ == '__main__':
    generate_launch_description()

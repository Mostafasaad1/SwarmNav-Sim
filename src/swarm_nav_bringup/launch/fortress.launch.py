#!/usr/bin/env python3
"""
fortress.launch.py
Unified Gazebo Fortress launch file for SwarmNav-Sim.
Spawns multiple robots with full topic bridges (cmd_vel, scan, odom, clock).
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import xacro


def spawn_robots(context, *args, **kwargs):
    """OpaqueFunction to dynamically spawn robots and their bridges."""
    num_robots = int(LaunchConfiguration('num_robots').perform(context))
    use_sim_time = LaunchConfiguration('use_sim_time').perform(context).lower() == 'true'
    
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')
    urdf_file = os.path.join(pkg_swarm_nav_bringup, 'urdf', 'swarm_robot.urdf.xacro')
    
    # Predefined positions for up to 5 robots
    positions = [
        (-15.0, -20.0, 0.1, 0.0), # x, y, z, yaw
        (-10.0, -20.0, 0.1, 0.0),
        (-5.0, -20.0, 0.1, 0.0),
        (0.0, -20.0, 0.1, 0.0),
        (5.0, -20.0, 0.1, 0.0),
    ]
    
    nodes = []
    
    for i in range(min(num_robots, 5)):
        robot_name = f'robot_{i}'
        x, y, z, yaw = positions[i]
        
        # 1. Process URDF with xacro (Python API is more robust for spawning)
        robot_description_config = xacro.process_file(
            urdf_file, 
            mappings={'robot_name': robot_name}
        )
        robot_desc = robot_description_config.toxml()
        
        # 2. Spawn robot in Gazebo
        # Using -topic for robot_description ensures Gazebo gets the full processed URDF
        spawn_robot = Node(
            package='ros_gz_sim',
            executable='create',
            name=f'spawn_{robot_name}',
            arguments=[
                '-name', robot_name,
                '-x', str(x),
                '-y', str(y),
                '-z', str(z),
                '-Y', str(yaw),
                '-string', robot_desc,
            ],
            output='screen'
        )
        
        # 3. Robot State Publisher
        robot_state_publisher = Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            namespace=robot_name,
            parameters=[{
                'robot_description': robot_desc,
                'use_sim_time': use_sim_time,
                'frame_prefix': f'{robot_name}/'
            }],
            remappings=[
                ('tf', '/tf'),
                ('tf_static', '/tf_static')
            ],
            output='screen'
        )
        
        # 4. Parameter Bridge for this robot
        # Bridging cmd_vel, scan, and odom
        bridge = Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name=f'bridge_{robot_name}',
            namespace=robot_name,
            arguments=[
                f'/{robot_name}/cmd_vel@geometry_msgs/msg/Twist@gz.msgs.Twist',
                f'/{robot_name}/odom@nav_msgs/msg/Odometry@gz.msgs.Odometry',
                f'/{robot_name}/scan@sensor_msgs/msg/LaserScan@gz.msgs.LaserScan',
                f'/model/{robot_name}/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
            ],
            remappings=[
                (f'/model/{robot_name}/tf', '/tf'),
            ],
            parameters=[{'use_sim_time': use_sim_time}],
            output='screen'
        )
        
        nodes.extend([spawn_robot, robot_state_publisher, bridge])
        
    return nodes


def generate_launch_description():
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')
    
    # Arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots', default_value='3', description='Number of robots to spawn'
    )
    
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time', default_value='true', description='Use simulation time'
    )
    
    gui_arg = DeclareLaunchArgument(
        'gui', default_value='true', description='Launch Gazebo GUI'
    )
    
    # Gazebo Sim launch
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('ros_gz_sim'),
                'launch',
                'gz_sim.launch.py'
            ])
        ]),
        launch_arguments={
            'gz_args': [
                PathJoinSubstitution([
                    pkg_swarm_nav_bringup, 'worlds', 'warehouse_gz.sdf'
                ]),
                ' -r'
            ],
        }.items()
    )
    
    # Clock Bridge
    clock_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='clock_bridge',
        arguments=['/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'],
        output='screen'
    )
    
    return LaunchDescription([
        num_robots_arg,
        use_sim_time_arg,
        gui_arg,
        gz_sim,
        clock_bridge,
        OpaqueFunction(function=spawn_robots)
    ])

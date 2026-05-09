#!/usr/bin/env python3
"""
Gazebo Fortress launch file for SwarmNav-Sim
Launches Gazebo with warehouse world and spawns multiple robots
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Get package directories
    pkg_swarm_nav_bringup = get_package_share_directory('swarm_nav_bringup')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')
    
    # Paths
    world_file = os.path.join(pkg_swarm_nav_bringup, 'worlds', 'warehouse.world')
    urdf_file = os.path.join(pkg_swarm_nav_bringup, 'urdf', 'swarm_robot.urdf.xacro')
    bridge_config = os.path.join(pkg_swarm_nav_bringup, 'config', 'bridge_config.yaml')
    
    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots to spawn'
    )
    
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation time'
    )
    
    gui_arg = DeclareLaunchArgument(
        'gui',
        default_value='true',
        description='Launch Gazebo GUI'
    )
    
    # Gazebo server
    gz_server = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('ros_gz_sim'),
                'launch',
                'gz_sim.launch.py'
            ])
        ]),
        launch_arguments={
            'gz_args': f'-r {world_file}',
            'on_exit_shutdown': 'true'
        }.items()
    )
    
    # Gazebo GUI (conditional)
    gz_gui = ExecuteProcess(
        cmd=['gz', 'sim', '-g'],
        output='screen',
        condition=IfCondition(LaunchConfiguration('gui'))
    )
    
    # Clock bridge (shared)
    clock_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='clock_bridge',
        output='screen',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock'
        ],
        parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}]
    )
    
    # Robot spawning and bridges will be added dynamically
    # For now, we'll create a static configuration for 3 robots
    # TODO: Make this dynamic based on num_robots parameter
    
    robot_spawns = []
    robot_bridges = []
    robot_state_publishers = []
    
    # Predefined spawn positions for up to 5 robots
    spawn_positions = [
        {'x': '-15.0', 'y': '-20.0', 'z': '0.1', 'yaw': '0.0'},
        {'x': '-10.0', 'y': '-20.0', 'z': '0.1', 'yaw': '0.0'},
        {'x': '-5.0', 'y': '-20.0', 'z': '0.1', 'yaw': '0.0'},
        {'x': '0.0', 'y': '-20.0', 'z': '0.1', 'yaw': '0.0'},
        {'x': '5.0', 'y': '-20.0', 'z': '0.1', 'yaw': '0.0'},
    ]
    
    # Create nodes for each robot (static for now - 3 robots)
    for i in range(3):
        robot_name = f'robot_{i}'
        pos = spawn_positions[i]
        
        # Process URDF with xacro
        xacro_cmd = ExecuteProcess(
            cmd=['xacro', urdf_file, f'robot_name:={robot_name}'],
            output='screen',
            name=f'xacro_{robot_name}'
        )
        
        # Spawn robot in Gazebo
        spawn_robot = Node(
            package='ros_gz_sim',
            executable='create',
            name=f'spawn_{robot_name}',
            output='screen',
            arguments=[
                '-name', robot_name,
                '-x', pos['x'],
                '-y', pos['y'],
                '-z', pos['z'],
                '-Y', pos['yaw'],
                '-file', urdf_file,
            ],
            parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}]
        )
        
        # Bridge for this robot
        robot_bridge = Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name=f'bridge_{robot_name}',
            namespace=robot_name,
            output='screen',
            arguments=[
                f'/{robot_name}/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
                f'/{robot_name}/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
                f'/{robot_name}/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            ],
            parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}],
            remappings=[
                ('cmd_vel', f'/{robot_name}/cmd_vel'),
                ('odom', f'/{robot_name}/odom'),
                ('scan', f'/{robot_name}/scan'),
            ]
        )
        
        # Robot state publisher
        robot_state_pub = Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            namespace=robot_name,
            output='screen',
            parameters=[
                {'use_sim_time': LaunchConfiguration('use_sim_time')},
                {'robot_description': ExecuteProcess(
                    cmd=['xacro', urdf_file, f'robot_name:={robot_name}'],
                    output='screen'
                )}
            ]
        )
        
        robot_spawns.append(spawn_robot)
        robot_bridges.append(robot_bridge)
        robot_state_publishers.append(robot_state_pub)
    
    # Build launch description
    ld = LaunchDescription()
    
    # Add arguments
    ld.add_action(num_robots_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(gui_arg)
    
    # Add Gazebo
    ld.add_action(gz_server)
    ld.add_action(gz_gui)
    
    # Add clock bridge
    ld.add_action(clock_bridge)
    
    # Add robot nodes
    for spawn, bridge, state_pub in zip(robot_spawns, robot_bridges, robot_state_publishers):
        ld.add_action(spawn)
        ld.add_action(bridge)
        ld.add_action(state_pub)
    
    return ld

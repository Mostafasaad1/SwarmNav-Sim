#!/usr/bin/env python3
"""
Main launch file for SwarmNav-Sim multi-robot system.

Launches all robots with SLAM, navigation, and coordination nodes.
"""

import os
import tempfile
import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    GroupAction,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.conditions import IfCondition, LaunchConfigurationEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, PushRosNamespace, LifecycleNode, SetRemap
from launch_ros.substitutions import FindPackageShare


def make_robot_nav2_params(nav2_config_path, robot_namespace):
    """Read the base yaml, substitute robot-specific values, write a temp file."""
    with open(nav2_config_path, 'r') as f:
        content = f.read()

    # String-replace all placeholder tokens
    content = content.replace('ROBOT_ODOM_FRAME', f'{robot_namespace}/odom')
    content = content.replace('base_footprint', f'{robot_namespace}/base_footprint')
    content = content.replace('odom_topic: odom', f'odom_topic: /{robot_namespace}/odom')
    content = content.replace('topic: /scan', f'topic: /{robot_namespace}/scan')
    # Wrap under robot namespace root key
    data = yaml.safe_load(content)
    wrapped = {robot_namespace: data}

    tmp = tempfile.NamedTemporaryFile(
        mode='w', suffix='.yaml', delete=False,
        prefix=f'nav2_{robot_namespace}_'
    )
    yaml.dump(wrapped, tmp)
    tmp.flush()
    return tmp.name


def generate_robot_launch(robot_id, x_pos, y_pos, yaw):
    """Generate launch actions for a single robot."""
    bringup_dir = get_package_share_directory('swarm_nav_bringup')

    # Robot namespace
    robot_namespace = f'robot_{robot_id}'

    # Nav2 configuration — generate a per-robot resolved yaml
    nav2_config = os.path.join(bringup_dir, 'config', 'robot_nav2.yaml')
    configured_params = make_robot_nav2_params(nav2_config, robot_namespace)

    # Group all robot nodes under namespace
    robot_group = GroupAction([
        PushRosNamespace(robot_namespace),
        SetRemap('tf', '/tf'),
        SetRemap('tf_static', '/tf_static'),

        # Static transform: map -> robot_N/odom (placeholder until SLAM provides it)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='static_map_odom',
            arguments=[
                str(x_pos), str(y_pos), '0',  # x y z
                str(yaw), '0', '0',           # yaw pitch roll
                'map',
                f'{robot_namespace}/odom'
            ],
            parameters=[{'use_sim_time': True}],
        ),

        # Graph merge node for multi-robot SLAM
        Node(
            package='swarm_nav_slam',
            executable='graph_merge_node',
            name='graph_merge_node',
            parameters=[{
                'use_sim_time': True,
                'robot_id': robot_namespace,
                'rendezvous_distance': 3.0,
                'shared_graph_topic': '/mrg_slam/shared_graph'
            }],
            output='screen'
        ),

        # Frontier detector node
        Node(
            package='swarm_nav_coordination',
            executable='frontier_detector_node',
            name='frontier_detector_node',
            parameters=[{
                'use_sim_time': True,
                'robot_id': robot_namespace,
                'min_frontier_size': 10,
                'frontier_travel_point_distance': 1.0
            }],
            output='screen'
        ),

        # Auctioneer node
        Node(
            package='swarm_nav_coordination',
            executable='auctioneer_node',
            name='auctioneer_node',
            parameters=[{
                'use_sim_time': True,
                'robot_id': robot_namespace,
                'bid_timeout_ms': 500,
                'nominal_speed': 0.5
            }],
            output='screen'
        ),

        # ORCA velocity filter node
        Node(
            package='swarm_nav_navigation',
            executable='orca_velocity_filter_node',
            name='orca_velocity_filter_node',
            parameters=[{
                'use_sim_time': True,
                'robot_id': robot_namespace,
                'robot_radius': 0.25,
                'max_neighbors': 10,
                'time_horizon': 2.0,
                'max_linear_velocity': 0.5,
                'max_angular_velocity': 1.0
            }],
            output='screen'
        ),

        # Nav2 Controller Server
        LifecycleNode(
            package='nav2_controller',
            executable='controller_server',
            name='controller_server',
            namespace='',
            output='screen',
            parameters=[configured_params],
        ),

        # Nav2 Planner Server
        LifecycleNode(
            package='nav2_planner',
            executable='planner_server',
            name='planner_server',
            namespace='',
            output='screen',
            parameters=[configured_params],
        ),

        # Nav2 Behavior Server
        LifecycleNode(
            package='nav2_behaviors',
            executable='behavior_server',
            name='behavior_server',
            namespace='',
            output='screen',
            parameters=[configured_params],
        ),

        # Nav2 BT Navigator
        LifecycleNode(
            package='nav2_bt_navigator',
            executable='bt_navigator',
            name='bt_navigator',
            namespace='',
            output='screen',
            parameters=[configured_params],
        ),

        # Nav2 Lifecycle Manager
        Node(
            package='nav2_lifecycle_manager',
            executable='lifecycle_manager',
            name='lifecycle_manager_navigation',
            output='screen',
            parameters=[{
                'use_sim_time': True,
                'autostart': True,
                'node_names': [
                    'controller_server',
                    'planner_server',
                    'behavior_server',
                    'bt_navigator'
                ]
            }]
        ),
    ])

    return robot_group


def launch_robots(context, *args, **kwargs):
    """OpaqueFunction: dynamically reads num_robots and spawns robot groups."""
    num_robots = int(LaunchConfiguration('num_robots').perform(context))
    num_robots = max(1, min(5, num_robots))  # Clamp to [1, 5]

    # Robot positions in warehouse (40m x 60m)
    robot_positions = [
        (0, -15.0, 0.0, 0.0),      # robot_0
        (-10.0, -15.0, 0.0, 0.0),  # robot_1
        (10.0, -15.0, 0.0, 0.0),   # robot_2
        (-5.0, -20.0, 0.0, 0.0),   # robot_3
        (5.0, -20.0, 0.0, 0.0),    # robot_4
    ]

    actions = []
    for i in range(num_robots):
        robot_id, x, y, yaw = robot_positions[i]
        actions.append(generate_robot_launch(i, x, y, yaw))

    return actions


def generate_launch_description():
    """Generate launch description for multi-robot swarm."""
    # Launch arguments
    num_robots_arg = DeclareLaunchArgument(
        'num_robots',
        default_value='3',
        description='Number of robots to spawn (1-5)'
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation time'
    )

    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='Launch RViz for visualization'
    )

    simulator_arg = DeclareLaunchArgument(
        'simulator',
        default_value='gazebo',
        description='Simulator backend to use (gazebo, coppeliasim, or none)'
    )

    # Get launch configurations
    use_rviz = LaunchConfiguration('use_rviz')

    # Include simulator launch file based on simulator argument
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('swarm_nav_bringup'),
                'launch',
                'harmonic.launch.py'
            ])
        ]),
        launch_arguments={
            'num_robots': LaunchConfiguration('num_robots'),
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }.items(),
        condition=LaunchConfigurationEquals('simulator', 'gazebo')
    )

    # Create launch description
    ld = LaunchDescription()

    # Add launch arguments
    ld.add_action(num_robots_arg)
    ld.add_action(use_sim_time_arg)
    ld.add_action(use_rviz_arg)
    ld.add_action(simulator_arg)

    # Add simulator launch
    ld.add_action(gazebo_launch)

    # Spawn robots dynamically based on num_robots parameter
    ld.add_action(OpaqueFunction(function=launch_robots))

    # Neighbor state aggregator node (collects individual states into array)
    ld.add_action(Node(
        package='swarm_nav_navigation',
        executable='neighbor_state_aggregator_node',
        name='neighbor_state_aggregator_node',
        parameters=[{
            'use_sim_time': True,
            'publish_rate': 10.0,
            'state_timeout': 0.5
        }],
        output='screen'
    ))

    # Obstacle tracker node (publishes dynamic obstacles)
    ld.add_action(Node(
        package='swarm_nav_navigation',
        executable='obstacle_tracker_node',
        name='obstacle_tracker_node',
        parameters=[{
            'use_sim_time': True,
            'publish_rate': 10.0,
            'obstacle_timeout': 2.0
        }],
        output='screen'
    ))

    # RViz for visualization
    bringup_dir = get_package_share_directory('swarm_nav_bringup')
    rviz_config = os.path.join(bringup_dir, 'rviz', 'swarm_nav.rviz')

    ld.add_action(Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        parameters=[{'use_sim_time': True}],
        condition=IfCondition(use_rviz),
        output='screen'
    ))

    return ld


if __name__ == '__main__':
    generate_launch_description()

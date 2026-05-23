from launch import LaunchDescription
from launch_ros.actions import Node, LifecycleNode, PushRosNamespace
from launch.actions import GroupAction
import launch
import launch_ros

def generate_launch_description():
    ld = LaunchDescription()
    
    group = GroupAction([
        PushRosNamespace('my_robot'),
        LifecycleNode(
            package='demo_nodes_cpp',
            executable='talker',
            name='my_talker',
            namespace='',
        )
    ])
    ld.add_action(group)
    return ld

if __name__ == '__main__':
    generate_launch_description()

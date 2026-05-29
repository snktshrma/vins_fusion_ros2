from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    default_config = PathJoinSubstitution([
        FindPackageShare('vins_fusion_ros2'),
        'config',
        'gazebo',
        'gazebo_stereo_config.yaml',
    ])

    return LaunchDescription([
        DeclareLaunchArgument(
            'config_file',
            default_value=default_config,
            description='Path to VINS YAML config',
        ),
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use /clock (true for Gazebo sim or rosbag --clock)',
        ),
        Node(
            package='vins_fusion_ros2',
            executable='vins_fusion_ros2_node',
            name='vins_estimator',
            namespace='vins_estimator',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'config_file': LaunchConfiguration('config_file'),
            }],
        ),
    ])

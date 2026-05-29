#!/usr/bin/env python3
"""VIO + loop closure (no global/GPS fusion in launch)."""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('vins_fusion_ros2')
    default_config = PathJoinSubstitution([
        pkg_share, 'config', 'gazebo', 'gazebo_stereo_config.yaml',
    ])

    config_file = LaunchConfiguration('config_file')
    use_sim_time = LaunchConfiguration('use_sim_time')
    enable_loop = LaunchConfiguration('enable_loop_fusion')

    vins_node = Node(
        package='vins_fusion_ros2',
        executable='vins_fusion_ros2_node',
        name='vins_estimator',
        namespace='vins_estimator',
        output='screen',
        emulate_tty=True,
        parameters=[{
            'use_sim_time': use_sim_time,
            'config_file': config_file,
            'world_frame_id': 'world',
            'body_frame_id': 'body',
            'camera_frame_id': 'camera',
        }],
    )

    loop_node = Node(
        package='vins_fusion_ros2',
        executable='loop_fusion_node',
        name='loop_fusion',
        namespace='loop_fusion',
        output='screen',
        arguments=[config_file],
        parameters=[{'use_sim_time': use_sim_time}],
        condition=IfCondition(enable_loop),
    )

    return LaunchDescription([
        DeclareLaunchArgument('config_file', default_value=default_config),
        DeclareLaunchArgument('use_sim_time', default_value='true'),
        DeclareLaunchArgument('enable_loop_fusion', default_value='true'),
        vins_node,
        loop_node,
    ])

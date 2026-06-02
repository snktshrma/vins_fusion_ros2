#!/usr/bin/env python3

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='true'),
        Node(
            package='vins_fusion_ros2',
            executable='voxel_map_node',
            name='voxel_map',
            output='screen',
            parameters=[{
                'use_sim_time': LaunchConfiguration('use_sim_time'),
                'odom_topic': '/loop_fusion/odometry_rect',
                'cloud_topic': '/camera/depth/points',
                'map_topic': '/voxel_map/occupancy',
                'map_frame': 'world',
                'voxel_size': 0.15,
                'input_leaf': 0.05,
                'window_size': 50.0,
                'publish_hz': 2.0,
            }],
        ),
    ])

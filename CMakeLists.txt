cmake_minimum_required(VERSION 3.5)
project(vins_fusion_ros2)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcpputils REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(message_filters REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)

find_package(OpenCV REQUIRED)
find_package(Ceres REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(vins)
add_subdirectory(camera_models)
add_subdirectory(loop_fusion)
add_subdirectory(global_fusion)

add_executable(vins_fusion_ros2_node
  src/vins_estimator.cpp
  src/vins_estimator_node.cpp
  src/visualization.cpp
)

ament_target_dependencies(vins_fusion_ros2_node
  rclcpp rcpputils message_filters std_msgs visualization_msgs
  geometry_msgs nav_msgs tf2 tf2_ros cv_bridge image_transport sensor_msgs)

target_link_libraries(vins_fusion_ros2_node vins_lib)

install(
  DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS vins_fusion_ros2_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()

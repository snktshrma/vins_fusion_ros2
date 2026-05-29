add_subdirectory(ThirdParty/GeographicLib)

add_executable(global_fusion_node
  src/globalOptNode.cpp
  src/globalOpt.cpp
)

target_include_directories(global_fusion_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../camera_models/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GeographicLib/include
  ${CERES_INCLUDE_DIRS}
)

ament_target_dependencies(global_fusion_node
  rclcpp
  ament_index_cpp
  nav_msgs
  geometry_msgs
  sensor_msgs
  visualization_msgs
)

target_link_libraries(global_fusion_node
  vins_camera_models
  ${CERES_LIBRARIES}
  libGeographiccc
)

install(TARGETS global_fusion_node DESTINATION lib/${PROJECT_NAME})

install(
  DIRECTORY models/
  DESTINATION share/${PROJECT_NAME}/models
)

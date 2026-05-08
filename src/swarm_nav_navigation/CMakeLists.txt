cmake_minimum_required(VERSION 3.8)
project(swarm_nav_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(nav2_core REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(pluginlib REQUIRED)

# Include directories
include_directories(include plugins)

# Obstacle tracker node
add_executable(obstacle_tracker_node src/obstacle_tracker_node.cpp)
ament_target_dependencies(obstacle_tracker_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
)

# ORCA velocity filter node
add_executable(orca_velocity_filter_node src/orca_velocity_filter_node.cpp)
ament_target_dependencies(orca_velocity_filter_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  tf2_ros
)

# Dynamic obstacle layer plugin
add_library(dynamic_obstacle_layer SHARED
  plugins/dynamic_obstacle_layer.cpp
)
ament_target_dependencies(dynamic_obstacle_layer
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  nav2_core
  nav2_costmap_2d
  pluginlib
)

# Install executables
install(TARGETS
  obstacle_tracker_node
  orca_velocity_filter_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install plugin library
install(TARGETS
  dynamic_obstacle_layer
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install plugin description
pluginlib_export_plugin_description_file(nav2_costmap_2d plugins/costmap_plugins.xml)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

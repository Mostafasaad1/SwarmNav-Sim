cmake_minimum_required(VERSION 3.8)
project(swarm_nav_navigation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(swarm_nav_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(pluginlib REQUIRED)
find_package(tf2_ros REQUIRED)

# Executables
add_executable(neighbor_state_aggregator_node src/neighbor_state_aggregator_node.cpp)
ament_target_dependencies(neighbor_state_aggregator_node
  rclcpp swarm_nav_msgs geometry_msgs
)

add_executable(orca_velocity_filter_node src/orca_velocity_filter_node.cpp)
ament_target_dependencies(orca_velocity_filter_node
  rclcpp swarm_nav_msgs geometry_msgs tf2_ros
)

add_executable(obstacle_tracker_node src/obstacle_tracker_node.cpp)
ament_target_dependencies(obstacle_tracker_node
  rclcpp swarm_nav_msgs nav_msgs geometry_msgs
)

# Costmap plugin
add_library(dynamic_obstacle_layer SHARED plugins/dynamic_obstacle_layer.cpp)
ament_target_dependencies(dynamic_obstacle_layer
  rclcpp nav2_costmap_2d pluginlib swarm_nav_msgs
)

install(TARGETS
  neighbor_state_aggregator_node
  orca_velocity_filter_node
  obstacle_tracker_node
  dynamic_obstacle_layer
  DESTINATION lib/${PROJECT_NAME}
)

install(FILES plugins.xml
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(test_orca_filter test/test_orca_filter.cpp)
  ament_target_dependencies(test_orca_filter rclcpp swarm_nav_msgs)
  
  ament_add_gtest(test_obstacle_tracker test/test_obstacle_tracker.cpp)
  ament_target_dependencies(test_obstacle_tracker rclcpp swarm_nav_msgs)
endif()

pluginlib_export_plugin_description_file(nav2_costmap_2d plugins.xml)
ament_package()

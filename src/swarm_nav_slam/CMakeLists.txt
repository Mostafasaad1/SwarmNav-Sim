cmake_minimum_required(VERSION 3.8)
project(swarm_nav_slam)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(swarm_nav_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

add_executable(graph_merge_node src/graph_merge_node.cpp)
ament_target_dependencies(graph_merge_node
  rclcpp
  swarm_nav_msgs
  nav_msgs
  geometry_msgs
  sensor_msgs
  tf2_ros
  tf2_geometry_msgs
)

install(TARGETS
  graph_merge_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(test_graph_merge test/test_graph_merge.cpp)
  target_link_libraries(test_graph_merge)
  ament_target_dependencies(test_graph_merge rclcpp swarm_nav_msgs nav_msgs sensor_msgs)
endif()

ament_package()

cmake_minimum_required(VERSION 3.8)
project(swarm_nav_coordination)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)

# Include directories
include_directories(include)

# Frontier detector node
add_executable(frontier_detector_node src/frontier_detector_node.cpp)
ament_target_dependencies(frontier_detector_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  visualization_msgs
)

# Auctioneer node
add_executable(auctioneer_node src/auctioneer_node.cpp)
ament_target_dependencies(auctioneer_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
)

# BehaviorTree nodes as shared library
add_library(swarm_bt_nodes SHARED
  src/bt_nodes/map_coverage_check.cpp
  src/bt_nodes/frontier_detector_bt.cpp
  src/bt_nodes/run_auction_bt.cpp
)
ament_target_dependencies(swarm_bt_nodes
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  behaviortree_cpp_v3
)

# Install executables
install(TARGETS
  frontier_detector_node
  auctioneer_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install BT plugin library
install(TARGETS
  swarm_bt_nodes
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install config files
install(DIRECTORY
  config
  DESTINATION share/${PROJECT_NAME}
)

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

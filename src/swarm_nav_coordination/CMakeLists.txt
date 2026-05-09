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
find_package(swarm_nav_msgs REQUIRED)

# Optional: BehaviorTree.CPP for BT nodes
find_package(behaviortree_cpp QUIET)

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
  swarm_nav_msgs
)

# Auctioneer node
add_executable(auctioneer_node src/auctioneer_node.cpp)
ament_target_dependencies(auctioneer_node
  rclcpp
  std_msgs
  geometry_msgs
  nav_msgs
  swarm_nav_msgs
)

# BehaviorTree nodes as shared library (only if BT.CPP is available)
if(behaviortree_cpp_FOUND)
  message(STATUS "BehaviorTree.CPP v4 found, building BT nodes")
  
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
    swarm_nav_msgs
    behaviortree_cpp
  )
  
  # Install BT plugin library
  install(TARGETS
    swarm_bt_nodes
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )
else()
  message(WARNING "BehaviorTree.CPP v4 not found, skipping BT nodes")
endif()

# Install executables
install(TARGETS
  frontier_detector_node
  auctioneer_node
  DESTINATION lib/${PROJECT_NAME}
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

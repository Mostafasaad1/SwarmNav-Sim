cmake_minimum_required(VERSION 3.8)
project(swarm_nav_coordination)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(swarm_nav_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)

# Executables
add_executable(auctioneer_node src/auctioneer_node.cpp)
ament_target_dependencies(auctioneer_node
  rclcpp swarm_nav_msgs geometry_msgs
)

add_executable(frontier_detector_node src/frontier_detector_node.cpp)
ament_target_dependencies(frontier_detector_node
  rclcpp swarm_nav_msgs nav_msgs geometry_msgs visualization_msgs
)

# BehaviorTree plugins
add_library(swarm_bt_nodes SHARED
  src/bt_nodes/map_coverage_check.cpp
  src/bt_nodes/frontier_detector_bt.cpp
  src/bt_nodes/run_auction_bt.cpp
)
ament_target_dependencies(swarm_bt_nodes
  rclcpp behaviortree_cpp swarm_nav_msgs nav_msgs
)

install(TARGETS
  auctioneer_node
  frontier_detector_node
  swarm_bt_nodes
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  ament_add_gtest(test_auctioneer test/test_auctioneer.cpp)
  ament_target_dependencies(test_auctioneer rclcpp swarm_nav_msgs)
endif()

ament_package()

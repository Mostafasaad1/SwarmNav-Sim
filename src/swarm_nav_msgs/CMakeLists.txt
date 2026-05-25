cmake_minimum_required(VERSION 3.8)
project(swarm_nav_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/AuctionAnnounce.msg"
  "msg/AuctionBid.msg"
  "msg/AuctionResult.msg"
  "msg/Frontier.msg"
  "msg/FrontierArray.msg"
  "msg/NeighborState.msg"
  "msg/NeighborStateArray.msg"
  "msg/Obstacle.msg"
  "msg/ObstacleArray.msg"
  DEPENDENCIES std_msgs geometry_msgs nav_msgs
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_dependencies(rosidl_default_runtime)
ament_package()

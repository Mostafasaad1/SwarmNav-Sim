cmake_minimum_required(VERSION 3.8)
project(swarm_nav_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

# Install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

# Install config files
install(DIRECTORY
  config
  DESTINATION share/${PROJECT_NAME}/
)

# Install world files
install(DIRECTORY
  worlds
  DESTINATION share/${PROJECT_NAME}/
)

# Install URDF files
install(DIRECTORY
  urdf
  DESTINATION share/${PROJECT_NAME}/
)

# Install RViz config
install(DIRECTORY
  rviz
  DESTINATION share/${PROJECT_NAME}/
)

# Install Python scripts
install(PROGRAMS
  swarm_nav_bringup/simple_explorer.py
  DESTINATION lib/${PROJECT_NAME}
)

# Install test files
install(DIRECTORY
  test
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

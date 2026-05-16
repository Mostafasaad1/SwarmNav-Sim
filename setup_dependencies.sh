#!/bin/bash
# setup_dependencies.sh
# Installs all dependencies for SwarmNav-Sim on ROS 2 Jazzy + Gazebo Harmonic
# Usage: ./setup_dependencies.sh [--dry-run] [--no-sim]

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

DRY_RUN=false
NO_SIM=false

for arg in "$@"; do
    case $arg in
        --dry-run) DRY_RUN=true; shift ;;
        --no-sim) NO_SIM=true; shift ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo "  --dry-run    Print actions without executing"
            echo "  --no-sim     Skip Gazebo installation"
            exit 0 ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            exit 1 ;;
    esac
done

run_cmd() {
    if [ "$DRY_RUN" = true ]; then
        echo -e "${YELLOW}[DRY-RUN]${NC} $*"
    else
        echo -e "${GREEN}[EXEC]${NC} $*"
        "$@"
    fi
}

is_installed() { dpkg -s "$1" &>/dev/null; }

echo "========================================="
echo "SwarmNav-Sim Dependency Setup (ROS 2 Jazzy)"
echo "========================================="
echo ""

# Check ROS 2 Jazzy
echo -e "${GREEN}[1/5]${NC} Checking ROS 2 Jazzy..."
if [ -z "$ROS_DISTRO" ]; then
    echo -e "${RED}ERROR: ROS 2 is not sourced. Please run:${NC}"
    echo "  source /opt/ros/jazzy/setup.bash"
    exit 1
elif [ "$ROS_DISTRO" != "jazzy" ]; then
    echo -e "${RED}ERROR: ROS_DISTRO is '$ROS_DISTRO', but 'jazzy' is required.${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} ROS 2 Jazzy detected"
echo ""

# Install BehaviorTree.CPP
echo -e "${GREEN}[2/5]${NC} Installing BehaviorTree.CPP..."
if is_installed "ros-jazzy-behaviortree-cpp"; then
    echo -e "${YELLOW}✓${NC} ros-jazzy-behaviortree-cpp already installed"
else
    run_cmd sudo apt-get update -qq
    run_cmd sudo apt-get install -y ros-jazzy-behaviortree-cpp
    echo -e "${GREEN}✓${NC} BehaviorTree.CPP installed"
fi
echo ""

# Install Nav2 + SLAM + RViz
echo -e "${GREEN}[3/5]${NC} Installing Nav2 + SLAM + RViz..."
NAV2_PACKAGES=(
    "ros-jazzy-navigation2"
    "ros-jazzy-nav2-bringup"
    "ros-jazzy-nav2-costmap-2d"
    "ros-jazzy-nav2-core"
    "ros-jazzy-nav2-util"
    "ros-jazzy-slam-toolbox"
    "ros-jazzy-xacro"
    "ros-jazzy-robot-state-publisher"
    "ros-jazzy-tf2-tools"
    "ros-jazzy-rviz2"
    "ros-jazzy-rmw-cyclonedds-cpp"
)

NAV2_MISSING=()
for pkg in "${NAV2_PACKAGES[@]}"; do
    if ! is_installed "$pkg"; then
        NAV2_MISSING+=("$pkg")
    fi
done

if [ ${#NAV2_MISSING[@]} -eq 0 ]; then
    echo -e "${YELLOW}✓${NC} All Nav2/SLAM packages already installed"
else
    echo "Installing ${#NAV2_MISSING[@]} packages..."
    run_cmd sudo apt-get update -qq
    run_cmd sudo apt-get install -y "${NAV2_MISSING[@]}"
    echo -e "${GREEN}✓${NC} Nav2 + SLAM + tools installed"
fi
echo ""

# Install Gazebo Harmonic + ros_gz bridge
if [ "$NO_SIM" = true ]; then
    echo -e "${GREEN}[4/5]${NC} Skipping Gazebo (--no-sim flag)"
else
    echo -e "${GREEN}[4/5]${NC} Installing Gazebo Harmonic + ros_gz bridge..."
    GAZEBO_PACKAGES=(
        "gz-harmonic"
        "ros-jazzy-ros-gz"
        "ros-jazzy-ros-gz-bridge"
        "ros-jazzy-ros-gz-sim"
        "ros-jazzy-ros-gz-interfaces"
    )

    GAZEBO_MISSING=()
    for pkg in "${GAZEBO_PACKAGES[@]}"; do
        if ! is_installed "$pkg"; then
            GAZEBO_MISSING+=("$pkg")
        fi
    done

    if [ ${#GAZEBO_MISSING[@]} -eq 0 ]; then
        echo -e "${YELLOW}✓${NC} All Gazebo packages already installed"
    else
        echo "Installing ${#GAZEBO_MISSING[@]} Gazebo packages..."
        run_cmd sudo apt-get update -qq
        run_cmd sudo apt-get install -y "${GAZEBO_MISSING[@]}"
        echo -e "${GREEN}✓${NC} Gazebo Harmonic installed"
    fi
fi
echo ""

# rosdep install
echo -e "${GREEN}[5/5]${NC} Installing workspace ROS dependencies via rosdep..."
run_cmd rosdep install --from-paths src --ignore-src -r -y
echo -e "${GREEN}✓${NC} rosdep dependencies installed"
echo ""

echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. source /opt/ros/jazzy/setup.bash"
echo "  2. ./build.sh"
echo "  3. source install/setup.bash"
echo "  4. ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3"
echo ""

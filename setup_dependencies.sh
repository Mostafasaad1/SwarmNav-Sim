#!/bin/bash
# setup_dependencies.sh
# Installs all optional dependencies for SwarmNav-Sim
# Usage: ./setup_dependencies.sh [--dry-run] [--no-sim] [--coppeliasim]

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse arguments
DRY_RUN=false
NO_SIM=false
COPPELIASIM=false

for arg in "$@"; do
    case $arg in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --no-sim)
            NO_SIM=true
            shift
            ;;
        --coppeliasim)
            COPPELIASIM=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --dry-run       Print actions without executing"
            echo "  --no-sim        Skip simulator installation (for CI without GPU)"
            echo "  --coppeliasim   Show CoppeliaSim setup instructions"
            echo "  --help, -h      Show this help message"
            echo ""
            echo "This script installs optional dependencies for SwarmNav-Sim:"
            echo "  - BehaviorTree.CPP v4"
            echo "  - Nav2 navigation stack"
            echo "  - Ignition Gazebo (Fortress) (unless --no-sim)"
            echo "  - ROS 2 package dependencies via rosdep"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $arg${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to run or print command
run_cmd() {
    if [ "$DRY_RUN" = true ]; then
        echo -e "${YELLOW}[DRY-RUN]${NC} $*"
    else
        echo -e "${GREEN}[EXEC]${NC} $*"
        "$@"
    fi
}

# Function to check if package is installed
is_installed() {
    dpkg -s "$1" &> /dev/null
}

echo "========================================="
echo "SwarmNav-Sim Dependency Setup"
echo "========================================="
echo ""

# Check ROS 2 Humble
echo -e "${GREEN}[1/5]${NC} Checking ROS 2 Humble..."
if [ -z "$ROS_DISTRO" ]; then
    echo -e "${RED}ERROR: ROS 2 is not sourced. Please run:${NC}"
    echo "  source /opt/ros/humble/setup.bash"
    exit 1
elif [ "$ROS_DISTRO" != "humble" ]; then
    echo -e "${RED}ERROR: ROS_DISTRO is '$ROS_DISTRO', but 'humble' is required.${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} ROS 2 Humble detected"
echo ""

# Install BehaviorTree.CPP v4
echo -e "${GREEN}[2/5]${NC} Installing BehaviorTree.CPP v4..."
if is_installed "ros-humble-behaviortree-cpp"; then
    echo -e "${YELLOW}✓${NC} ros-humble-behaviortree-cpp already installed (skipping)"
else
    run_cmd sudo apt-get update
    run_cmd sudo apt-get install -y ros-humble-behaviortree-cpp
    echo -e "${GREEN}✓${NC} BehaviorTree.CPP v4 installed"
fi
echo ""

# Install Nav2 stack
echo -e "${GREEN}[3/5]${NC} Installing Nav2 navigation stack..."
NAV2_PACKAGES=(
    "ros-humble-navigation2"
    "ros-humble-nav2-bringup"
    "ros-humble-nav2-costmap-2d"
    "ros-humble-pluginlib"
)

NAV2_MISSING=()
for pkg in "${NAV2_PACKAGES[@]}"; do
    if ! is_installed "$pkg"; then
        NAV2_MISSING+=("$pkg")
    fi
done

if [ ${#NAV2_MISSING[@]} -eq 0 ]; then
    echo -e "${YELLOW}✓${NC} All Nav2 packages already installed (skipping)"
else
    echo "Installing ${#NAV2_MISSING[@]} Nav2 packages..."
    run_cmd sudo apt-get update
    run_cmd sudo apt-get install -y "${NAV2_MISSING[@]}"
    echo -e "${GREEN}✓${NC} Nav2 stack installed"
fi
echo ""

# Install Gazebo Ignition (Fortress)
if [ "$NO_SIM" = true ]; then
    echo -e "${GREEN}[4/5]${NC} Skipping Gazebo (--no-sim flag)"
else
    echo -e "${GREEN}[4/5]${NC} Installing Gazebo Ignition (Fortress)..."
    GAZEBO_PACKAGES=(
        "ignition-fortress"
        "ros-humble-ros-gz"
        "ros-humble-ros-gz-bridge"
        "ros-humble-ros-gz-sim"
    )
    
    GAZEBO_MISSING=()
    for pkg in "${GAZEBO_PACKAGES[@]}"; do
        if ! is_installed "$pkg"; then
            GAZEBO_MISSING+=("$pkg")
        fi
    done
    
    if [ ${#GAZEBO_MISSING[@]} -eq 0 ]; then
        echo -e "${YELLOW}✓${NC} All Gazebo packages already installed (skipping)"
    else
        echo "Installing ${#GAZEBO_MISSING[@]} Gazebo packages..."
        run_cmd sudo apt-get update
        run_cmd sudo apt-get install -y "${GAZEBO_MISSING[@]}"
        echo -e "${GREEN}✓${NC} Gazebo Ignition (Fortress) installed"
    fi
fi
echo ""

# Run rosdep
echo -e "${GREEN}[5/5]${NC} Installing ROS 2 package dependencies..."
run_cmd rosdep install --from-paths src --ignore-src -r -y
echo -e "${GREEN}✓${NC} Package dependencies installed"
echo ""

# CoppeliaSim instructions
if [ "$COPPELIASIM" = true ]; then
    echo "========================================="
    echo "CoppeliaSim Setup Instructions"
    echo "========================================="
    echo ""
    echo "CoppeliaSim is an optional alternative simulator backend (Priority P3)."
    echo ""
    echo "1. Download CoppeliaSim EDU from:"
    echo "   https://www.coppeliarobotics.com/downloads"
    echo ""
    echo "2. Extract and set environment variable:"
    echo "   export COPPELIASIM_ROOT_DIR=/path/to/CoppeliaSim"
    echo ""
    echo "3. Clone and build simROS2 interface:"
    echo "   cd \$COPPELIASIM_ROOT_DIR/programming"
    echo "   git clone --recursive https://github.com/CoppeliaRobotics/simROS2.git"
    echo "   cd simROS2"
    echo "   colcon build --symlink-install"
    echo ""
    echo "4. Copy compiled plugin to CoppeliaSim:"
    echo "   cp install/sim_ros2_interface/lib/libsimROS2.so \$COPPELIASIM_ROOT_DIR/"
    echo ""
    echo "5. Launch with CoppeliaSim backend:"
    echo "   ros2 launch swarm_nav_bringup swarm.launch.py simulator:=coppeliasim"
    echo ""
fi

echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "  1. Build the workspace:"
echo "     colcon build --symlink-install"
echo ""
echo "  2. Source the workspace:"
echo "     source install/setup.bash"
echo ""
echo "  3. Launch the simulation:"
echo "     ros2 launch swarm_nav_bringup swarm.launch.py simulator:=gazebo num_robots:=3"
echo ""

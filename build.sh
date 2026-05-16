#!/usr/bin/env bash
# build.sh — Automated robust build script for SwarmNav-Sim
# Bypasses the colcon "sh-extension environment" bug by calling CMake/PIP directly.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Source ROS 2 base
source /opt/ros/humble/setup.bash

# Create install directory if it doesn't exist
mkdir -p install
INSTALL_DIR="$(pwd)/install"

# Define package priority (messages first)
PRIORITY_PACKAGES=("swarm_nav_msgs")

JOBS=$(nproc)
echo "=== SwarmNav-Sim Robust Build (${JOBS} jobs) ==="

function build_pkg() {
    local pkg_path=$1
    local pkg_name=$(basename "$pkg_path")
    
    echo ""
    echo "--- Building ${pkg_name} ---"
    
    local build_root="$(pwd)/build/${pkg_name}"
    
    if [ -f "${pkg_path}/setup.py" ]; then
        echo "Installing Python package..."
        pip install --prefix="${INSTALL_DIR}" "${pkg_path}"
    elif [ -f "${pkg_path}/CMakeLists.txt" ]; then
        mkdir -p "${build_root}"
        cd "${build_root}"
        
        cmake "${SCRIPT_DIR}/${pkg_path}" \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
            -DAMENT_PREFIX_PATH="${INSTALL_DIR};/opt/ros/humble" \
            -DCMAKE_PREFIX_PATH="${INSTALL_DIR};/opt/ros/humble" \
            -DBUILD_TESTING=ON
            
        make -j"${JOBS}"
        make install
        cd "${SCRIPT_DIR}"
    fi
    
    # Source environment to resolve dependencies for next packages
    if [ -f "${INSTALL_DIR}/setup.bash" ]; then
        source "${INSTALL_DIR}/setup.bash"
    fi
}

# 1. Build priority packages
for pkg in "${PRIORITY_PACKAGES[@]}"; do
    pkg_path=$(find src -maxdepth 2 -name "$pkg" -type d | head -n 1)
    if [ -n "$pkg_path" ]; then
        build_pkg "$pkg_path"
    fi
done

# 2. Build all other packages
# Find all directories in src/ that have CMakeLists.txt or setup.py and aren't in PRIORITY_PACKAGES
ALL_PKGS=$(find src -maxdepth 3 -name "package.xml" -exec dirname {} \;)

for pkg_path in $ALL_PKGS; do
    pkg_name=$(basename "$pkg_path")
    
    # Skip if already built
    is_priority=false
    for p in "${PRIORITY_PACKAGES[@]}"; do
        if [[ "$pkg_name" == "$p" ]]; then is_priority=true; break; fi
    done
    
    if [ "$is_priority" = false ]; then
        build_pkg "$pkg_path"
    fi
done

echo ""
echo "=== All packages built and installed successfully ==="
echo ""
echo "To launch the simulation (Bash):"
echo "  source install/setup.bash"
echo "  ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3"
echo ""
echo "To launch the simulation (Zsh):"
echo "  source install/setup.zsh"
echo "  ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3"

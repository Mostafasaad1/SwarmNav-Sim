#!/usr/bin/env bash
# build.sh — Robust build script for SwarmNav-Sim (ROS 2 Jazzy)
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

ROS_SETUP=/opt/ros/jazzy/setup.bash
if [ ! -f "$ROS_SETUP" ]; then
    echo "ERROR: Cannot find $ROS_SETUP — is ROS 2 Jazzy installed?"
    exit 1
fi

source "$ROS_SETUP"

# Source existing install if it exists (for incremental builds)
if [ -f install/setup.bash ]; then
    source install/setup.bash
fi

JOBS=2
echo "=== SwarmNav-Sim build ($JOBS jobs, ROS 2 Jazzy) ==="

unset RMW_IMPLEMENTATION

colcon build \
    --symlink-install \
    --parallel-workers "$JOBS" \
    --cmake-args -DCMAKE_BUILD_TYPE=Release \
    --event-handlers console_cohesion+

echo ""
echo "=== Build complete ==="
echo ""
echo "To launch the simulation:"
echo "  source install/setup.bash"
echo "  ros2 launch swarm_nav_bringup swarm.launch.py num_robots:=3"

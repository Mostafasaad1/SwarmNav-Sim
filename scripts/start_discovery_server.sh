#!/bin/bash
# Start ROS 2 Discovery Server for SwarmNav-Sim
# This script starts a FastDDS discovery server on localhost:11811

echo "Starting ROS 2 Discovery Server..."
echo "Server ID: 0"
echo "Address: 127.0.0.1:11811"
echo ""
echo "To connect clients, set in each terminal:"
echo "  export ROS_DISCOVERY_SERVER=127.0.0.1:11811"
echo ""

fastdds discovery -i 0 -l 127.0.0.1 -p 11811

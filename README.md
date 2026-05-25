<div align="center">
  <h1>SwarmNav-Sim 🤖🤖🤖</h1>
  <p><strong>A ROS 2 Jazzy multi-robot warehouse exploration & navigation simulation</strong></p>

  [![ROS 2](https://img.shields.io/badge/ROS_2-Jazzy-blue.svg)](https://docs.ros.org/en/jazzy/)
  [![Gazebo](https://img.shields.io/badge/Gazebo-Fortress-orange.svg)](https://gazebosim.org/home)
  [![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
  
  <br />

  <!-- 🎥 REPLACE THE LINK BELOW WITH YOUR DEMO VIDEO OR GIF -->
  <img src="https://via.placeholder.com/800x450?text=Insert+SwarmNav-Sim+Demo+Video/GIF+Here" alt="SwarmNav-Sim Demo" width="800"/>
</div>

<hr />

## 📖 Overview

**SwarmNav-Sim** is an advanced multi-robot simulation environment built on ROS 2 Jazzy and Gazebo Fortress. It orchestrates a swarm of differential drive robots (3 to 5 units) performing autonomous exploration and navigation within a dynamic 40m × 60m warehouse environment. 

The system leverages state-of-the-art decentralized coordination:
* **Frontier-based Exploration**: Robots detect unexplored boundaries and expand their maps dynamically.
* **Auction-based Task Allocation**: A decentralized bidding system assigns frontiers to the most suitable robots based on distance and traversal cost.
* **ORCA Collision Avoidance**: Real-time velocity filtering ensures safe, collision-free maneuvers even among dense clusters of robots and dynamic obstacles.

## ✨ Features

- **ROS 2 Lifecycle Nodes**: Robust state management for system bringup and teardown.
- **BehaviorTree.CPP (BT) Integration**: Modular task execution and behavioral logic for individual robots.
- **Multi-Robot Graph Merging**: Real-time SLAM occupancy grid overlay for seamless map merging upon robot rendezvous.
- **Dynamic Obstacle Tracking**: Built-in tracking of dynamic entities (forklifts, humans) shared across the swarm.
- **Metrics Evaluation**: Automated coverage tracking, collision monitoring, and SLAM accuracy evaluation.

---

## 🛠️ Architecture

The system is separated into highly cohesive ROS 2 packages:

- `swarm_nav_msgs`: Custom message definitions for swarm communication (bids, auctions, neighbor states).
- `swarm_nav_slam`: Graph-based SLAM and map merging coordinators.
- `swarm_nav_coordination`: BT plugins for frontier detection and auction participation.
- `swarm_nav_navigation`: Velocity obstacles (ORCA) filter and Nav2 stack configuration.
- `swarm_nav_evaluation`: Automated coverage and performance metric generation.

> For a complete architecture breakdown, including topic graphs and behavior algorithms, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 🚀 Quickstart

### Prerequisites

- **OS**: Ubuntu 24.04
- **ROS 2**: Jazzy Jalisco
- **Simulator**: Gazebo Fortress

### Installation

Clone the repository and install dependencies:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone https://github.com/Mostafasaad1/SwarmNav-Sim.git
cd ~/ros2_ws
rosdep update
rosdep install --from-paths src -y --ignore-src
colcon build --symlink-install
```

### Running the Simulation

Source your workspace and launch the core bringup file:

```bash
source install/setup.bash
ros2 launch swarm_nav_bringup swarm.launch.py robots:=3
```

By default, this will spawn 3 robots. You can change the number of robots (up to 5) by tweaking the `robots` argument.

---

## 📷 Media Gallery

*(Add more screenshots of RViz, Gazebo, or node graphs here)*

<p align="center">
  <img src="https://via.placeholder.com/400x250?text=RViz+Map+Merging+Screenshot" width="45%" />
  <img src="https://via.placeholder.com/400x250?text=Gazebo+Warehouse+Screenshot" width="45%" />
</p>

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

# SwarmNav-Sim Quick Reference Card

One-page reference for common commands and workflows.

---

## 🚀 Quick Start (3 Commands)

```bash
./setup_dependencies.sh          # 1. Install (first time only)
colcon build                     # 2. Build
source install/setup.zsh         # 3. Source (every terminal)
```

---

## 🎮 Launch Commands

### Full System (Recommended)
```bash
ros2 launch swarm_nav_bringup swarm.launch.py \
  simulator:=gazebo num_robots:=3 use_rviz:=true
```

### Variations
```bash
# 5 robots
ros2 launch swarm_nav_bringup swarm.launch.py simulator:=gazebo num_robots:=5

# Headless (no GUI)
ros2 launch swarm_nav_bringup swarm.launch.py simulator:=gazebo use_rviz:=false

# Evaluation
ros2 launch swarm_nav_evaluation evaluation.launch.py num_robots:=3 duration:=300
```

---

## 🧪 Testing

```bash
colcon test                      # Run all tests
colcon test-result --verbose     # View results
colcon test --packages-select swarm_nav_slam  # Test one package
```

---

## 📊 Monitoring

```bash
# List topics
ros2 topic list

# Monitor specific topics
ros2 topic echo /robot_0/odom
ros2 topic echo /robot_0/frontiers
ros2 topic echo /swarm/neighbor_states

# Check topic frequency
ros2 topic hz /robot_0/scan

# List nodes
ros2 node list

# Node info
ros2 node info /robot_0/auctioneer_node
```

---

## 🔧 Debugging

```bash
# View logs
ros2 topic echo /rosout

# TF tree
ros2 run tf2_tools view_frames

# RViz (if not launched)
ros2 run rviz2 rviz2 -d src/swarm_nav_bringup/rviz/swarm_nav.rviz

# Kill everything
killall -9 ros2 gz rviz2
```

---

## 📁 Key Files

```
src/swarm_nav_bringup/
├── launch/
│   ├── swarm.launch.py          # Main launch file
│   ├── gazebo.launch.py         # Simulator
│   └── evaluation.launch.py     # Metrics
├── config/
│   ├── robot_nav2.yaml          # Navigation params
│   └── mrg_slam_multirobot.yaml # SLAM config
├── worlds/
│   └── warehouse.world          # Gazebo world
└── urdf/
    └── swarm_robot.urdf.xacro   # Robot model
```

---

## 🎯 Key Topics

### Per-Robot
- `/robot_X/scan` - Lidar data
- `/robot_X/odom` - Position/velocity
- `/robot_X/map` - SLAM map
- `/robot_X/frontiers` - Detected frontiers
- `/robot_X/cmd_vel` - Velocity commands

### Swarm-Level
- `/swarm/neighbor_states` - All robot states
- `/swarm/tracked_obstacles` - Dynamic obstacles
- `/swarm/auction/*` - Task allocation

---

## ⚙️ Common Parameters

### In swarm.launch.py
```python
num_robots:=3           # Number of robots (1-5)
simulator:=gazebo       # Simulator (gazebo/none)
use_rviz:=true         # Launch RViz
use_sim_time:=true     # Use simulation time
```

### In robot_nav2.yaml
```yaml
max_vel_x: 0.5         # Max linear velocity
max_vel_theta: 1.0     # Max angular velocity
```

### In ORCA filter
```python
robot_radius: 0.25     # Robot size
time_horizon: 2.0      # Avoidance lookahead
max_neighbors: 10      # Max robots to consider
```

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| "No module named 'swarm_nav_msgs'" | `colcon build --packages-select swarm_nav_msgs && source install/setup.zsh` |
| "package 'ros_gz_sim' not found" | `./setup_dependencies.sh` |
| Gazebo crashes | `killall gz` then relaunch |
| Robots don't move | Check `ros2 topic hz /robot_0/cmd_vel` |
| High CPU | Reduce robots or disable RViz |

---

## 📚 Documentation

- **QUICKSTART.md** - 5-minute guide
- **RUNNING_INSTRUCTIONS.md** - Complete guide
- **ARCHITECTURE.md** - System design
- **TUNING.md** - Parameter tuning
- **README.md** - Project overview

---

## 🔄 Typical Workflow

```bash
# 1. Open terminal
cd ~/projects/SwarmNav-Sim

# 2. Source ROS 2 and workspace
source /opt/ros/humble/setup.zsh
source install/setup.zsh

# 3. Launch system
ros2 launch swarm_nav_bringup swarm.launch.py simulator:=gazebo num_robots:=3

# 4. In new terminal, monitor
source install/setup.zsh
ros2 topic list
ros2 topic echo /robot_0/odom

# 5. Stop with Ctrl+C
```

---

## 💡 Tips

- **Always source** the workspace in new terminals
- **Use zsh setup** if using zsh shell (`setup.zsh` not `setup.bash`)
- **Start with 3 robots** for best performance
- **Enable RViz** to visualize what's happening
- **Check logs** if something doesn't work
- **Kill cleanly** with Ctrl+C, not killall

---

## 🎓 Learning Path

1. **Day 1**: Run QUICKSTART.md, watch robots explore
2. **Day 2**: Read ARCHITECTURE.md, understand design
3. **Day 3**: Modify parameters, see effects
4. **Day 4**: Run evaluation, analyze metrics
5. **Day 5**: Add custom features

---

## 📞 Getting Help

1. Check **RUNNING_INSTRUCTIONS.md** troubleshooting section
2. Review **ARCHITECTURE.md** for design understanding
3. Check ROS 2 logs: `ros2 topic echo /rosout`
4. Verify topics: `ros2 topic list`
5. Check node status: `ros2 node list`

---

**Print this page for quick reference while working!**

---

**Last Updated**: May 9, 2026

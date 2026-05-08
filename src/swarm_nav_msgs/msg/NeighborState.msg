# NeighborState.msg
# Broadcasted by each robot for ORCA collision avoidance

string robot_id                 # Namespace ID (e.g., "robot_1")
geometry_msgs/Pose pose         # Current localized pose
geometry_msgs/Twist velocity    # Current velocity
float32 radius                  # Footprint radius

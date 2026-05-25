# Obstacle.msg
# Represents a dynamic or semi-dynamic entity in the environment

string id                       # Unique identifier (e.g., "human_1", "forklift_2")
geometry_msgs/Pose pose         # Current position and orientation
geometry_msgs/Twist velocity    # Current linear and angular velocity
float32 radius                  # Collision radius
uint8 classification            # STATIC=0, SEMI_DYNAMIC=1, DYNAMIC=2

# Classification constants
uint8 STATIC = 0
uint8 SEMI_DYNAMIC = 1
uint8 DYNAMIC = 2

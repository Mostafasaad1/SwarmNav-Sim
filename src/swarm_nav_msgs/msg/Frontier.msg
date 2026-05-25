# Frontier.msg
# Represents an unexplored boundary in the map

geometry_msgs/Point centroid    # 3D coordinate of the frontier center
float32 size                    # Number of cells comprising the frontier
float32 utility                 # Estimated information gain
string frontier_id              # Unique UUID for tracking

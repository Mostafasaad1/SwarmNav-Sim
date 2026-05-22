import os

def replace_in_file(path, old, new):
    with open(path, 'r') as f:
        content = f.read()
    content = content.replace(old, new)
    with open(path, 'w') as f:
        f.write(content)

# collision_monitor.py
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/collision_monitor.py', 
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node""",
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node""")

# coverage_evaluator.py
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/coverage_evaluator.py', 
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import OccupancyGrid
from rclpy.node import Node""",
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import OccupancyGrid
from rclpy.node import Node""")

# slam_metrics.py
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/slam_metrics.py',
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node""",
"""import json

import numpy as np
import rclpy
from nav_msgs.msg import Odometry
from rclpy.node import Node""")

# test_collision_monitor.py
replace_in_file('src/swarm_nav_evaluation/test/test_collision_monitor.py',
"""import pytest
import math
from geometry_msgs.msg import Pose""",
"""import math

import pytest
from geometry_msgs.msg import Pose""")
replace_in_file('src/swarm_nav_evaluation/test/test_collision_monitor.py', 'f"threshold={threshold})"', "f'threshold={threshold})'")
replace_in_file('src/swarm_nav_evaluation/test/test_collision_monitor.py', 'f"threshold={threshold})"', "f'threshold={threshold})'")

# test_coverage_evaluator.py
replace_in_file('src/swarm_nav_evaluation/test/test_coverage_evaluator.py',
"""import pytest
from nav_msgs.msg import OccupancyGrid
import numpy as np
import json""",
"""from nav_msgs.msg import OccupancyGrid
import pytest""")
replace_in_file('src/swarm_nav_evaluation/test/test_coverage_evaluator.py', 'f"Expected 0% coverage for empty grid, got {coverage}%"', "f'Expected 0% coverage for empty grid, got {coverage}%'")

# bayesian_tuner.py
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/bayesian_tuner.py', 'import json\n', '')
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/bayesian_tuner.py', 'import time\nfrom datetime import datetime', 'from datetime import datetime\nimport time')
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/bayesian_tuner.py', 'import yaml\n\nfrom swarm_nav_evaluation.benchmark_runner import BenchmarkRunner', 'from swarm_nav_evaluation.benchmark_runner import BenchmarkRunner\nimport yaml')

# benchmark_runner.py
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/benchmark_runner.py', 'import time\nfrom copy import deepcopy', 'from copy import deepcopy\nimport time')
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/benchmark_runner.py', 'import yaml\n\nfrom swarm_nav_evaluation.report_utils import', 'from swarm_nav_evaluation.report_utils import')
replace_in_file('src/swarm_nav_evaluation/swarm_nav_evaluation/benchmark_runner.py', "f\"FAIL-FAST: Scenario {scenario['id']} \"", "f'FAIL-FAST: Scenario {scenario[\"id\"]} '")

# setup.py
replace_in_file('src/swarm_nav_evaluation/setup.py', 'import os\nfrom glob import glob', 'from glob import glob\nimport os')


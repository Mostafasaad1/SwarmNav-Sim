import os
import glob
import re

files = glob.glob('src/swarm_nav_evaluation/swarm_nav_evaluation/*.py') + glob.glob('src/swarm_nav_evaluation/test/*.py')

for f in files:
    with open(f, 'r') as file:
        content = file.read()
    
    # Fix double quotes to single quotes where simple (not containing single quotes)
    content = re.sub(r'"([^"\']+)"', r"'\1'", content)
    
    # Fix import order in specific files by just doing naive replace
    if 'collision_monitor.py' in f or 'coverage_evaluator.py' in f or 'slam_metrics.py' in f:
        content = content.replace("from nav_msgs.msg", "import json\nimport numpy as np\nfrom nav_msgs.msg")
        content = content.replace("import numpy as np\nimport json\n", "")
        
    with open(f, 'w') as file:
        file.write(content)

import re
import sys

def fix_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    out_lines = []
    for idx, line in enumerate(lines):
        # Fix C-style casts
        if "(int)" in line:
            line = line.replace("(int)", "static_cast<int>(") + ")" if not line.endswith(")\n") else line.replace("(int)", "static_cast<int>(")
        
        # Fix whitespace before comments
        if "  // " not in line and " //" in line:
            line = line.replace(" //", "  //")
            
        out_lines.append(line)
        
    with open(filepath, 'w') as f:
        f.writelines(out_lines)

import glob
files = glob.glob('src/swarm_nav_coordination/src/**/*.cpp', recursive=True) + glob.glob('src/swarm_nav_coordination/test/**/*.cpp', recursive=True)
for f in files:
    fix_file(f)

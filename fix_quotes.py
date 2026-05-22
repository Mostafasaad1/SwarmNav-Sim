import os
import glob

files = glob.glob('src/swarm_nav_evaluation/swarm_nav_evaluation/*.py') + glob.glob('src/swarm_nav_evaluation/test/*.py')

for f in files:
    with open(f, 'r') as file:
        content = file.read()
    
    # Revert my bad script
    content = content.replace('""\'', '"""')
    content = content.replace('\'""', '"""')
        
    with open(f, 'w') as file:
        file.write(content)

import os
import glob

copyright = """// Copyright 2026 SwarmNav-Sim Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

"""

cpp_files = glob.glob('src/swarm_nav_coordination/src/**/*.cpp', recursive=True) + glob.glob('src/swarm_nav_coordination/test/**/*.cpp', recursive=True)

for f in cpp_files:
    with open(f, 'r') as file:
        content = file.read()
    
    if "Copyright" not in content[:200]:
        content = copyright + content
        with open(f, 'w') as file:
            file.write(content)

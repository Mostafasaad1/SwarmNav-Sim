from glob import glob
import os

from setuptools import find_packages, setup

package_name = 'swarm_nav_evaluation'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools', 'psutil', 'pandas', 'optuna'],
    zip_safe=True,
    maintainer='mox',
    maintainer_email='mostafa.saad.1tb@gmail.com',
    description='Evaluation tools for SwarmNav-Sim',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'coverage_evaluator.py = swarm_nav_evaluation.coverage_evaluator:main',
            'collision_monitor.py = swarm_nav_evaluation.collision_monitor:main',
            'slam_metrics.py = swarm_nav_evaluation.slam_metrics:main',
            'timer_shutdown.py = swarm_nav_evaluation.timer_shutdown:main',
            'benchmark_runner.py = swarm_nav_evaluation.benchmark_runner:main',
            'bayesian_tuner.py = swarm_nav_evaluation.bayesian_tuner:main',
            'system_metrics.py = swarm_nav_evaluation.system_metrics:main',
        ],
    },
)

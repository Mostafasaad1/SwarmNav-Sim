#!/usr/bin/env python3
"""
test_collision_monitor.py
Unit tests for collision monitor
"""

import pytest
import math
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Point, Pose


def calculate_distance(pose1, pose2):
    """Calculate Euclidean distance between two poses"""
    dx = pose1.position.x - pose2.position.x
    dy = pose1.position.y - pose2.position.y
    return math.sqrt(dx * dx + dy * dy)


def test_collision_detection_below_threshold():
    """Test collision detection when distance is below threshold"""
    
    # Create two poses close together
    pose1 = Pose()
    pose1.position.x = 0.0
    pose1.position.y = 0.0
    
    pose2 = Pose()
    pose2.position.x = 0.3
    pose2.position.y = 0.0
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    assert collision_detected, f"Expected collision (distance={distance:.2f} < threshold={threshold})"
    assert distance == pytest.approx(0.3, abs=0.01)


def test_collision_detection_above_threshold():
    """Test no collision when distance is above threshold"""
    
    pose1 = Pose()
    pose1.position.x = 0.0
    pose1.position.y = 0.0
    
    pose2 = Pose()
    pose2.position.x = 1.0
    pose2.position.y = 0.0
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    assert not collision_detected, f"Expected no collision (distance={distance:.2f} >= threshold={threshold})"
    assert distance == pytest.approx(1.0, abs=0.01)


def test_collision_detection_exact_threshold():
    """Test collision detection at exact threshold"""
    
    pose1 = Pose()
    pose1.position.x = 0.0
    pose1.position.y = 0.0
    
    pose2 = Pose()
    pose2.position.x = 0.5
    pose2.position.y = 0.0
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    # At exact threshold, should NOT be collision (< not <=)
    assert not collision_detected, f"Expected no collision at exact threshold (distance={distance:.2f})"


def test_collision_detection_diagonal():
    """Test collision detection with diagonal distance"""
    
    pose1 = Pose()
    pose1.position.x = 0.0
    pose1.position.y = 0.0
    
    pose2 = Pose()
    pose2.position.x = 0.3
    pose2.position.y = 0.4
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    # Distance should be 0.5 (3-4-5 triangle)
    assert distance == pytest.approx(0.5, abs=0.01)
    assert not collision_detected, "Expected no collision at exact threshold"


def test_collision_detection_same_position():
    """Test collision detection when robots are at same position"""
    
    pose1 = Pose()
    pose1.position.x = 1.0
    pose1.position.y = 2.0
    
    pose2 = Pose()
    pose2.position.x = 1.0
    pose2.position.y = 2.0
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    assert collision_detected, "Expected collision when robots at same position"
    assert distance == pytest.approx(0.0, abs=0.01)


def test_collision_detection_far_apart():
    """Test no collision when robots are far apart"""
    
    pose1 = Pose()
    pose1.position.x = 0.0
    pose1.position.y = 0.0
    
    pose2 = Pose()
    pose2.position.x = 10.0
    pose2.position.y = 10.0
    
    distance = calculate_distance(pose1, pose2)
    threshold = 0.5
    
    collision_detected = distance < threshold
    
    assert not collision_detected, f"Expected no collision (distance={distance:.2f} >> threshold={threshold})"
    assert distance > 10.0


if __name__ == '__main__':
    pytest.main([__file__, '-v'])

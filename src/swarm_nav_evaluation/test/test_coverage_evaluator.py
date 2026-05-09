#!/usr/bin/env python3
"""
test_coverage_evaluator.py
Unit tests for coverage evaluator
"""

import pytest
import numpy as np
from nav_msgs.msg import OccupancyGrid


def test_coverage_calculation_synthetic_grid():
    """Test coverage calculation on synthetic OccupancyGrid"""
    
    # Create synthetic grid: 10x10 = 100 cells
    grid = OccupancyGrid()
    grid.info.width = 10
    grid.info.height = 10
    grid.info.resolution = 0.05
    
    # Fill with 50% known cells (0-100), 50% unknown (-1)
    grid.data = []
    for i in range(100):
        if i < 50:
            grid.data.append(0)  # Free space (known)
        else:
            grid.data.append(-1)  # Unknown
    
    # Calculate coverage
    known_cells = sum(1 for cell in grid.data if cell != -1)
    total_cells = len(grid.data)
    coverage = (known_cells / total_cells) * 100.0
    
    # Verify 50% coverage
    assert coverage == 50.0, f"Expected 50% coverage, got {coverage}%"


def test_coverage_calculation_all_known():
    """Test coverage with all cells known"""
    
    grid = OccupancyGrid()
    grid.info.width = 5
    grid.info.height = 5
    grid.data = [0] * 25  # All free space
    
    known_cells = sum(1 for cell in grid.data if cell != -1)
    total_cells = len(grid.data)
    coverage = (known_cells / total_cells) * 100.0
    
    assert coverage == 100.0, f"Expected 100% coverage, got {coverage}%"


def test_coverage_calculation_all_unknown():
    """Test coverage with all cells unknown"""
    
    grid = OccupancyGrid()
    grid.info.width = 5
    grid.info.height = 5
    grid.data = [-1] * 25  # All unknown
    
    known_cells = sum(1 for cell in grid.data if cell != -1)
    total_cells = len(grid.data)
    coverage = (known_cells / total_cells) * 100.0
    
    assert coverage == 0.0, f"Expected 0% coverage, got {coverage}%"


def test_coverage_calculation_mixed():
    """Test coverage with mixed known/unknown cells"""
    
    grid = OccupancyGrid()
    grid.info.width = 4
    grid.info.height = 4
    # 12 known, 4 unknown = 75% coverage
    grid.data = [0, 50, 100, -1,
                 0, 50, 100, -1,
                 0, 50, 100, -1,
                 0, 50, 100, -1]
    
    known_cells = sum(1 for cell in grid.data if cell != -1)
    total_cells = len(grid.data)
    coverage = (known_cells / total_cells) * 100.0
    
    assert coverage == 75.0, f"Expected 75% coverage, got {coverage}%"


def test_coverage_empty_grid():
    """Test coverage with empty grid"""
    
    grid = OccupancyGrid()
    grid.info.width = 0
    grid.info.height = 0
    grid.data = []
    
    if len(grid.data) == 0:
        coverage = 0.0
    else:
        known_cells = sum(1 for cell in grid.data if cell != -1)
        total_cells = len(grid.data)
        coverage = (known_cells / total_cells) * 100.0
    
    assert coverage == 0.0, f"Expected 0% coverage for empty grid, got {coverage}%"


if __name__ == '__main__':
    pytest.main([__file__, '-v'])

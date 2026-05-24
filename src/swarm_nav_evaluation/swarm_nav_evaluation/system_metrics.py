#!/usr/bin/env python3
"""System metrics collector using psutil.

Collects CPU, memory, and RTF metrics during benchmark runs.
"""

import threading
import time
from typing import Dict, List, Optional

import psutil


class SystemMetricsCollector:
    """Collects system metrics (CPU, memory, RTF) during a benchmark run."""

    def __init__(self, interval: float = 1.0):
        """Initialize the collector with a sampling interval in seconds."""
        self.interval = interval
        self.samples: List[Dict] = []
        self._thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()

    def start(self) -> None:
        """Start collecting metrics in a background thread."""
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._collect_loop, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        """Stop collecting metrics."""
        self._stop_event.set()
        if self._thread:
            self._thread.join(timeout=2.0)

    def _collect_loop(self) -> None:
        """Background loop to collect metrics."""
        while not self._stop_event.is_set():
            sample = {
                'timestamp': time.time(),
                'cpu_percent': psutil.cpu_percent(interval=None),
                'memory_percent': psutil.virtual_memory().percent,
                'memory_used_mb': psutil.virtual_memory().used / (1024 * 1024),
            }
            self.samples.append(sample)
            time.sleep(self.interval)

    def get_average_cpu(self) -> float:
        """Return average CPU usage across all samples."""
        if not self.samples:
            return 0.0
        return sum(s['cpu_percent'] for s in self.samples) / len(self.samples)

    def get_average_memory_mb(self) -> float:
        """Return average memory usage in MB."""
        if not self.samples:
            return 0.0
        return sum(s['memory_used_mb'] for s in self.samples) / len(self.samples)

    def get_peak_cpu(self) -> float:
        """Return peak CPU usage."""
        if not self.samples:
            return 0.0
        return max(s['cpu_percent'] for s in self.samples)

    def get_peak_memory_mb(self) -> float:
        """Return peak memory usage in MB."""
        if not self.samples:
            return 0.0
        return max(s['memory_used_mb'] for s in self.samples)

    def to_dict(self) -> Dict:
        """Return a summary dict of collected metrics."""
        return {
            'avg_cpu_percent': self.get_average_cpu(),
            'avg_memory_mb': self.get_average_memory_mb(),
            'peak_cpu_percent': self.get_peak_cpu(),
            'peak_memory_mb': self.get_peak_memory_mb(),
            'sample_count': len(self.samples),
        }


def main(args=None):
    """Entry point for the system_metrics ROS 2 node."""
    import rclpy
    from rclpy.node import Node

    class SystemMetricsNode(Node):
        """ROS 2 node that runs the system metrics collector."""

        def __init__(self):
            """Initialize the node and start the collector."""
            super().__init__('system_metrics')
            self.declare_parameter('interval', 1.0)
            interval = self.get_parameter('interval').value
            self.collector = SystemMetricsCollector(interval=interval)
            self.collector.start()
            self.get_logger().info(
                f'System metrics collector started (interval={interval}s)'
            )

        def destroy_node(self):
            """Stop collector before destroying node."""
            self.collector.stop()
            super().destroy_node()

    rclpy.init(args=args)
    node = SystemMetricsNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

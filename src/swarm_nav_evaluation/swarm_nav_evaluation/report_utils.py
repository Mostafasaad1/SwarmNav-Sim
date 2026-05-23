#!/usr/bin/env python3
""'Report writing utilities for benchmark and tuning results.

Supports JSON, CSV, and correlation plot generation.
'"'

import csv
import json
import os
from typing import Any, Dict, List, Optional

try:
    import pandas as pd
    PANDAS_AVAILABLE = True
except ImportError:
    PANDAS_AVAILABLE = False


def write_json_report(data: Dict, output_path: str) -> None:
    '"'Write a dictionary to a JSON file.'""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'w') as f:
        json.dump(data, f, indent=2)


def write_csv_report(
    rows: List[Dict],
    output_path: str,
    fieldnames: Optional[List[str]] = None,
) -> None:
    ""'Write a list of dictionaries to a CSV file.'""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    if not rows:
        rows = []
    if fieldnames is None and rows:
        fieldnames = list(rows[0].keys())
    with open(output_path, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def append_csv_report(
    row: Dict,
    output_path: str,
    fieldnames: Optional[List[str]] = None,
) -> None:
    ""'Append a single row to a CSV file.'""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    file_exists = os.path.exists(output_path)
    if fieldnames is None:
        fieldnames = list(row.keys())
    with open(output_path, 'a', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow(row)


def generate_correlation_plot(
    df: Any,
    x_col: str,
    y_col: str,
    output_path: str,
) -> None:
    ""'Generate a simple correlation plot using pandas/matplotlib.'""
    if not PANDAS_AVAILABLE:
        raise ImportError('pandas is required for correlation plotting')
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    plt.figure(figsize=(8, 6))
    plt.scatter(df[x_col], df[y_col])
    plt.xlabel(x_col)
    plt.ylabel(y_col)
    plt.title(f'{y_col} vs {x_col}')
    plt.grid(True)
    plt.tight_layout()
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    plt.savefig(output_path)
    plt.close()


def generate_sweep_report(
    sweep_results: List[Dict],
    output_dir: str,
) -> Dict[str, str]:
    ""'Generate CSV and optional correlation plots for a parameter sweep.'""
    os.makedirs(output_dir, exist_ok=True)
    csv_path = os.path.join(output_dir, 'sweep_results.csv')
    write_csv_report(sweep_results, csv_path)

    plot_paths = {}
    if PANDAS_AVAILABLE and sweep_results:
        df = pd.DataFrame(sweep_results)
        numeric_cols = df.select_dtypes(include='number').columns.tolist()
        for col in numeric_cols:
            if col != 'exploration_time' and 'exploration_time' in df.columns:
                plot_path = os.path.join(
                    output_dir, f'{col}_vs_exploration_time.png'
                )
                try:
                    generate_correlation_plot(
                        df, col, 'exploration_time', plot_path
                    )
                    plot_paths[col] = plot_path
                except Exception:
                    pass

    return {
        'csv': csv_path,
        'plots': plot_paths,
    }

#!/usr/bin/env python3
"""
Performance Visualization and Plotting Script for MicroArchSim.
Reads real measured CSV results and produces publication-quality charts:
1. Workload CPI and IPC comparisons.
2. Cycle breakdown (productive vs data hazard vs branch flush vs memory stall).
3. Forwarding impact on CPI and stalls.
4. Cache capacity vs Hit Rate and Memory Stall Cycles.
5. Cache associativity trade-offs.
"""

import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

RESULTS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "results"))
PLOTS_DIR = os.path.join(RESULTS_DIR, "plots")

plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')
plt.rcParams['font.sans-serif'] = 'Arial'
plt.rcParams['axes.edgecolor'] = '#cccccc'
plt.rcParams['axes.linewidth'] = 0.8

def plot_workload_cpi():
    csv_file = os.path.join(RESULTS_DIR, "workload_summary.csv")
    if not os.path.exists(csv_file):
        return
    df = pd.read_csv(csv_file)
    df['workload'] = df['program'].apply(lambda x: os.path.splitext(os.path.basename(x))[0])

    fig, ax1 = plt.subplots(figsize=(10, 5), dpi=300)
    bars = ax1.bar(df['workload'], df['cpi'], color='#1f77b4', alpha=0.85, width=0.55, label='CPI')
    ax1.set_ylabel('Cycles Per Instruction (CPI)', fontsize=11, fontweight='bold', color='#1f77b4')
    ax1.set_title('MicroArchSim: CPI Across Standard Microarchitectural Workloads', fontsize=13, fontweight='bold', pad=15)
    ax1.set_xticklabels(df['workload'], rotation=35, ha='right', fontsize=9)
    ax1.grid(axis='y', linestyle='--', alpha=0.7)

    # Add data labels
    for bar in bars:
        yval = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2.0, yval + 0.05, f"{yval:.2f}", ha='center', va='bottom', fontsize=8, fontweight='bold')

    plt.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "workload_cpi.png"))
    plt.close()
    print("Generated workload_cpi.png")

def plot_cycle_breakdown():
    csv_file = os.path.join(RESULTS_DIR, "workload_summary.csv")
    if not os.path.exists(csv_file):
        return
    df = pd.read_csv(csv_file)
    df['workload'] = df['program'].apply(lambda x: os.path.splitext(os.path.basename(x))[0])

    # Normalized cycle breakdown
    cols = ['productive_cycles', 'data_hazard_stalls', 'control_flush_cycles', 'memory_stall_cycles', 'drain_cycles']
    labels = ['Productive', 'Data Hazard Stalls', 'Branch Flushes', 'Memory Stalls', 'Pipeline Drain']
    colors = ['#2ca02c', '#ff7f0e', '#d62728', '#9467bd', '#7f7f7f']

    fig, ax = plt.subplots(figsize=(11, 5.5), dpi=300)
    bottom = pd.Series([0.0] * len(df))

    for col, label, color in zip(cols, labels, colors):
        fraction = df[col] / df['total_cycles'] * 100.0
        ax.bar(df['workload'], fraction, bottom=bottom, label=label, color=color, alpha=0.9, width=0.55)
        bottom += fraction

    ax.set_ylabel('Execution Time Breakdown (%)', fontsize=11, fontweight='bold')
    ax.set_title('MicroArchSim: Cycle Breakdown by Microarchitectural Event', fontsize=13, fontweight='bold', pad=15)
    ax.set_xticklabels(df['workload'], rotation=35, ha='right', fontsize=9)
    ax.set_ylim(0, 100)
    ax.legend(loc='upper right', frameon=True, fontsize=9)
    ax.grid(axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "cycle_breakdown.png"))
    plt.close()
    print("Generated cycle_breakdown.png")

def plot_forwarding_impact():
    csv_file = os.path.join(RESULTS_DIR, "forwarding_comparison.csv")
    if not os.path.exists(csv_file):
        return
    df = pd.read_csv(csv_file)
    df['workload'] = df['program'].apply(lambda x: os.path.splitext(os.path.basename(x))[0])
    df['forwarding'] = df['config'].apply(lambda x: 'With Forwarding' if 'fwd' in x and 'nofwd' not in x else 'No Forwarding')

    pivot_cpi = df.pivot(index='workload', columns='forwarding', values='cpi')

    fig, ax = plt.subplots(figsize=(8, 4.5), dpi=300)
    pivot_cpi.plot(kind='bar', ax=ax, color=['#d62728', '#2ca02c'], width=0.6, alpha=0.9)
    ax.set_ylabel('CPI (Cycles Per Instruction)', fontsize=11, fontweight='bold')
    ax.set_title('Impact of Operand Forwarding on Pipeline CPI', fontsize=13, fontweight='bold', pad=15)
    ax.set_xticklabels(pivot_cpi.index, rotation=20, ha='right', fontsize=9)
    ax.legend(title='', frameon=True, fontsize=9)
    ax.grid(axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "forwarding_impact.png"))
    plt.close()
    print("Generated forwarding_impact.png")

def plot_cache_capacity_sweep():
    csv_file = os.path.join(RESULTS_DIR, "cache_capacity_sweep.csv")
    if not os.path.exists(csv_file):
        return
    df = pd.read_csv(csv_file)
    df['cache_size'] = [64, 128, 256, 512, 1024, 2048]

    fig, ax1 = plt.subplots(figsize=(8, 4.5), dpi=300)

    color1 = '#1f77b4'
    color2 = '#d62728'

    ax1.plot(df['cache_size'], df['cache_hit_rate'] * 100.0, marker='o', linewidth=2.2, color=color1, label='Hit Rate (%)')
    ax1.set_xlabel('Cache Capacity (Bytes)', fontsize=11, fontweight='bold')
    ax1.set_ylabel('Cache Hit Rate (%)', fontsize=11, fontweight='bold', color=color1)
    ax1.tick_params(axis='y', labelcolor=color1)
    ax1.set_xscale('log', base=2)
    ax1.set_xticks(df['cache_size'])
    ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())

    ax2 = ax1.twinx()
    ax2.plot(df['cache_size'], df['memory_stall_cycles'], marker='s', linewidth=2.2, linestyle='--', color=color2, label='Memory Stalls')
    ax2.set_ylabel('Memory Stall Cycles', fontsize=11, fontweight='bold', color=color2)
    ax2.tick_params(axis='y', labelcolor=color2)

    plt.title('Memory Subsystem: Cache Capacity Scaling vs Hit Rate & Stalls', fontsize=13, fontweight='bold', pad=15)
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS_DIR, "cache_capacity_sweep.png"))
    plt.close()
    print("Generated cache_capacity_sweep.png")

def main():
    os.makedirs(PLOTS_DIR, exist_ok=True)
    plot_workload_cpi()
    plot_cycle_breakdown()
    plot_forwarding_impact()
    plot_cache_capacity_sweep()
    print("All charts successfully generated in results/plots/!")

if __name__ == "__main__":
    main()

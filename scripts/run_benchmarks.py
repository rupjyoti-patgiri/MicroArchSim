#!/usr/bin/env python3
"""
Benchmark runner script for MicroArchSim.
Executes comprehensive architectural experiments across workloads, forwarding modes,
and cache configurations, saving real measured metrics into CSV files.
"""

import subprocess
import os
import glob
import csv

SIM_BIN = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build_test", "microarchsim.exe"))
PROGRAMS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "programs"))
RESULTS_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "results"))

def run_sim(program_path, extra_args=None, csv_output=None):
    cmd = [SIM_BIN, "--program", program_path]
    if extra_args:
        cmd.extend(extra_args)
    if csv_output:
        cmd.extend(["--stats-csv", csv_output])
    
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print(f"Error running {program_path}: {result.stderr}")
    return result

def run_workload_benchmarks():
    print("=== Running Baseline Workload Benchmarks ===")
    programs = sorted(glob.glob(os.path.join(PROGRAMS_DIR, "*.asm")))
    out_csv = os.path.join(RESULTS_DIR, "workload_summary.csv")
    if os.path.exists(out_csv):
        os.remove(out_csv)

    for p in programs:
        p_name = os.path.basename(p)
        print(f"Running {p_name}...")
        # Pipeline with default cache
        run_sim(p, ["--mode", "pipeline"], csv_output=out_csv)
    print(f"Workload summary written to {out_csv}\n")

def run_forwarding_comparison():
    print("=== Running Forwarding vs Non-Forwarding Experiment ===")
    out_csv = os.path.join(RESULTS_DIR, "forwarding_comparison.csv")
    if os.path.exists(out_csv):
        os.remove(out_csv)

    for p in ["01_independent_arithmetic.asm", "02_dependent_arithmetic.asm", "03_load_use_hazards.asm"]:
        p_path = os.path.join(PROGRAMS_DIR, p)
        # Forwarding ON
        run_sim(p_path, ["--mode", "pipeline", "--forwarding", "on", "--no-cache"], csv_output=out_csv)
        # Forwarding OFF
        run_sim(p_path, ["--mode", "pipeline", "--forwarding", "off", "--no-cache"], csv_output=out_csv)
    print(f"Forwarding comparison written to {out_csv}\n")

def run_cache_capacity_sweep():
    print("=== Running Cache Capacity Sweep Experiment ===")
    out_csv = os.path.join(RESULTS_DIR, "cache_capacity_sweep.csv")
    if os.path.exists(out_csv):
        os.remove(out_csv)

    prog = os.path.join(PROGRAMS_DIR, "10_large_working_set.asm")
    sizes = [64, 128, 256, 512, 1024, 2048]
    for sz in sizes:
        print(f"Testing cache size {sz} bytes...")
        run_sim(prog, ["--mode", "pipeline", "--cache-size", str(sz), "--associativity", "1", "--line-size", "16"], csv_output=out_csv)
    print(f"Cache capacity sweep written to {out_csv}\n")

def run_cache_associativity_study():
    print("=== Running Cache Associativity Study ===")
    out_csv = os.path.join(RESULTS_DIR, "cache_associativity_study.csv")
    if os.path.exists(out_csv):
        os.remove(out_csv)

    prog = os.path.join(PROGRAMS_DIR, "10_large_working_set.asm")
    assocs = [1, 2, 4]
    for a in assocs:
        print(f"Testing associativity {a}-way...")
        run_sim(prog, ["--mode", "pipeline", "--cache-size", "256", "--associativity", str(a), "--line-size", "16"], csv_output=out_csv)
    print(f"Cache associativity study written to {out_csv}\n")

def main():
    os.makedirs(RESULTS_DIR, exist_ok=True)
    run_workload_benchmarks()
    run_forwarding_comparison()
    run_cache_capacity_sweep()
    run_cache_associativity_study()
    print("All benchmark experiments completed successfully!")

if __name__ == "__main__":
    main()

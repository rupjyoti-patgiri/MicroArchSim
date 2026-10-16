# MicroArchSim: Cycle-Accurate CPU Pipeline and Memory Hierarchy Simulator

[![Build and Test](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![GoogleTest](https://img.shields.io/badge/Tests-GoogleTest-orange.svg)]()
[![Architecture](https://img.shields.io/badge/Arch-RV32I--Edu-purple.svg)]()

> **MicroArchSim** is an industry-quality, cycle-accurate microarchitectural simulator modeling instruction-level execution, in-order 5-stage pipelining, RAW data forwarding, load-use stall injection, control hazard branch resolution, and parameterized multi-level cache memory hierarchies.

Built from scratch in clean, modern C++17 to demonstrate core computer architecture principles relevant to CPU/GPU architecture, performance modeling, and hardware systems engineering.

---

## Architecture Overview

```
+=============================================================================+
|                                MICROARCHSIM                                 |
+=============================================================================+
|                                                                             |
|   +----+  IF/ID  +----+  ID/EX  +----+  EX/MEM +----+  MEM/WB +----+        |
|   | IF | ======> | ID | ======> | EX | ======> | MEM| ======> | WB |        |
|   +----+         +----+         +----+         +----+         +----+        |
|     ^              |              |              |              |           |
|     |              |       +------+------+       |              |           |
|     |              |       |   Forward   | <-----+              |           |
|     |              |       |    Unit     | <--------------------+           |
|     |              |       +-------------+                                  |
|     |              v                                                        |
|     |       +-------------+                                                 |
|     |       | Hazard Unit | (Load-use stall / branch flush detection)       |
|     |       +-------------+                                                 |
|     |                                                                       |
|     +------------------ L1 Data Cache (Set-Associative) --------------------+
|                                  |                                          |
|                                  v                                          |
|                             Main Memory                                     |
+=============================================================================+
```

### Key Technical Highlights
- **Supported ISA**: RV32I-Edu (17 instructions: `ADD`, `SUB`, `AND`, `OR`, `XOR`, `SLT`, `ADDI`, `ANDI`, `SLTI`, `LUI`, `AUIPC`, `LW`, `SW`, `BEQ`, `BNE`, `JAL`, `HALT`).
- **Correctness Oracle**: Dual-engine simulation pairing a single-instruction sequential reference interpreter against the 5-stage cycle-accurate pipeline.
- **Hazard Handling**:
  - Full RAW forwarding (`EX/MEM` $\rightarrow$ `EX` and `MEM/WB` $\rightarrow$ `EX`) with strict age prioritization.
  - Store-data forwarding into $rs2$ memory operands.
  - Hardware load-use hazard stall bubble injection.
  - Static predict-not-taken branch resolution in `EX` (2-cycle flush penalty) and `JAL` resolution in `ID` (1-cycle flush penalty).
- **Integrated Memory Hierarchy**:
  - Parameterized blocking cache (configurable capacity, cache-line size, associativity, replacement policy [LRU, FIFO, Random], and write policies [Write-Back/Write-Through, Write-Allocate/No-Write-Allocate]).
  - True pipeline stall cycles on cache misses with atomic memory refills.
- **Automated Verification**: 8 GoogleTest test suites covering instruction decoding, register invariants, reference semantics, hazard bypasses, cache replacement, memory stall delays, and differential testing.
- **Python Plotting Suite**: Automated generation of publication-quality architectural charts from real simulator CSV outputs.

---

## Directory Structure

```
MicroArchSim/
├── CMakeLists.txt             # Root CMake configuration
├── README.md                  # Comprehensive project documentation
├── LICENSE                    # MIT License
├── docs/                      # Architectural specifications
│   ├── architecture.md        # ISA specification and register conventions
│   ├── pipeline-design.md     # 5-stage pipeline and register boundaries
│   ├── hazard-handling.md     # Forwarding paths, load-use, and branch flush
│   ├── memory-hierarchy.md    # Cache geometry, replacement, and timing
│   └── performance-analysis.md# Metrics definitions and benchmark tables
├── include/                   # Public C++ header files
│   ├── assembler.h            # Two-pass assembler for RV32I-Edu
│   ├── cache.h                # Parameterized L1 cache simulator
│   ├── config.h               # Tunable simulator configuration parameters
│   ├── hazard_unit.h          # Hazard detection and operand forwarding
│   ├── instruction.h          # Instruction format, decoding, and ALU ops
│   ├── memory.h               # Flat byte-addressable memory model
│   ├── pipeline.h             # Cycle-stepped 5-stage pipeline engine
│   ├── pipeline_registers.h   # Explicit IF/ID, ID/EX, EX/MEM, MEM/WB registers
│   ├── reference_interpreter.h# Sequential golden reference oracle
│   ├── register_file.h        # 32-entry register file with x0 invariance
│   ├── statistics.h           # Cycle classification and CSV exporter
│   └── types.h                # Word, Addr, opcode, and enum definitions
├── src/                       # Simulator implementation source files
├── tests/                     # GoogleTest automated unit & differential test suites
├── programs/                  # 11 Microarchitectural benchmark assembly workloads
├── scripts/                   # Experiment automation & Matplotlib plotting scripts
│   ├── run_benchmarks.py      # Automated benchmark execution harness
│   └── plot_results.py        # Chart generation from CSV outputs
└── results/                   # Experimental CSV data and generated charts
    ├── workload_summary.csv
    ├── forwarding_comparison.csv
    ├── cache_capacity_sweep.csv
    ├── cache_associativity_study.csv
    └── plots/                 # High-resolution architectural performance plots
```

---

## Getting Started

### Prerequisites
- C++17 compatible compiler (`g++` $\ge 9$, `clang++` $\ge 10$, or MSVC $\ge 2019$)
- CMake $\ge 3.16$
- Ninja or Make
- Python 3 with `matplotlib` and `pandas`

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/hemanth/MicroArchSim.git
cd MicroArchSim

# Configure and compile core simulator and GoogleTest suite
cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTS=ON
cmake --build build
```

### Running Tests
Execute all 8 GoogleTest test suites via CTest:
```bash
ctest --test-dir build --output-on-failure
```

---

## Command-Line Usage

```bash
# Run a program in pipelined mode with default cache and forwarding
./build/microarchsim --program programs/02_dependent_arithmetic.asm --mode pipeline

# Run in reference mode (sequential oracle)
./build/microarchsim --program programs/11_matrix_mult.asm --mode reference --dump-registers

# Run with cycle-by-cycle pipeline tracing enabled
./build/microarchsim --program programs/03_load_use_hazards.asm --trace

# Run with custom cache configuration and CSV export
./build/microarchsim --program programs/07_sequential_memory.asm \
                    --cache-size 2048 \
                    --associativity 2 \
                    --line-size 32 \
                    --hit-latency 1 \
                    --miss-penalty 12 \
                    --stats-csv results/my_run.csv
```

---

## Benchmark Suite and Workload Analysis

MicroArchSim includes 11 targeted assembly workloads located in `programs/`:
1. `01_independent_arithmetic.asm`: Peak theoretical throughput test (CPI $\approx 1.0$).
2. `02_dependent_arithmetic.asm`: Consecutive RAW hazards isolating forwarding effectiveness.
3. `03_load_use_hazards.asm`: Load-use pairs isolating hardware bubble injection.
4. `04_store_forwarding.asm`: Store operations testing store-data bypass paths.
5. `05_branch_taken_not_taken.asm`: Direct comparison of branch penalty (0 vs 2 cycles).
6. `06_branch_heavy_loop.asm`: Tight loop evaluating branch penalty overhead on CPI.
7. `07_sequential_memory.asm`: Spatial locality demonstration across contiguous cache lines.
8. `08_strided_memory.asm`: 64-byte stride access defeating spatial locality.
9. `09_small_working_set.asm`: Working set fitting within cache (high hit rate).
10. `10_large_working_set.asm`: Working set exceeding cache capacity (capacity/conflict evictions).
11. `11_matrix_mult.asm`: Full compute/memory kernel computing $C_{2\times2} = A_{2\times2} \times B_{2\times2}$.

### Running Benchmarks and Plotting
```bash
# Execute all benchmark experiments and output CSVs to results/
python scripts/run_benchmarks.py

# Generate publication-quality figures
python scripts/plot_results.py
```

Generated plots in `results/plots/`:
- `workload_cpi.png`: Measured CPI across all 11 benchmark workloads.
- `cycle_breakdown.png`: Stacked percentage breakdown of productive vs stall vs flush vs memory cycles.
- `forwarding_impact.png`: CPI comparison with and without operand forwarding.
- `cache_capacity_sweep.png`: Cache hit rate and memory stall cycles scaling with capacity.

---

## Architectural Comparison: Forwarding Impact

Measured performance on consecutive RAW dependent instructions (`02_dependent_arithmetic.asm`):

| Configuration | Total Cycles | Retired Instructions | CPI | Data Stalls | IPC |
|:---|:---:|:---:|:---:|:---:|:---:|
| **With Forwarding (Bypassing)** | **15** | 11 | **1.36** | **0** | **0.73** |
| **Without Forwarding** | **24** | 11 | **2.18** | **9** | **0.46** |

Operand forwarding reduces execution latency by **37.5%**, completely eliminating 9 stall cycles.

---

## Limitations and Future Extensions
1. **Branch Predictor**: Currently implements static predict-not-taken. Future extensions: 2-bit saturating counters, Bimodal, or GShare branch predictors.
2. **Non-Blocking Cache (MSHR)**: Currently implements a blocking cache where misses freeze the pipeline. Future extensions: Miss Status Holding Registers (MSHRs) for hit-under-miss memory parallelism.
3. **Multi-Level Cache**: L2 and L3 shared cache hierarchy with inclusive/exclusive directory protocols.
4. **Superscalar / Out-of-Order Execution**: Dual-issue in-order dispatch or Tomasulo's algorithm with Reservation Stations and Reorder Buffer (ROB).

---

## License
MIT License. Created for computer architecture research, education, and technical interview preparation.

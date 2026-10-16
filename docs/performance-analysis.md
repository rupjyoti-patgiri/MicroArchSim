# Performance Metrics and Mathematical Definitions

## 1. Primary Metrics

### Total Cycles ($T_{cycles}$)
Total number of discrete clock cycles elapsed from the initial instruction fetch until the pipeline has completely drained following retirement of `HALT`.

### Instructions Retired ($N_{retired}$)
Count of valid instructions that successfully commit their architectural state in the Write-Back (`WB`) stage. Speculatively fetched instructions that are later flushed due to branch mispredictions are explicitly excluded.

### Cycles Per Instruction (CPI)

$$\text{CPI} = \frac{T_{cycles}}{N_{retired}}$$

*Edge-case handling*: If $N_{retired} = 0$, $\text{CPI} \equiv 0.0$.

### Instructions Per Cycle (IPC)

$$\text{IPC} = \frac{1}{\text{CPI}} = \frac{N_{retired}}{T_{cycles}}$$

*Edge-case handling*: If $T_{cycles} = 0$, $\text{IPC} \equiv 0.0$.

---

## 2. Pipeline Cycle Breakdown

Total simulated execution cycles decompose into distinct, mutually exclusive categories without double counting:

$$T_{cycles} = T_{productive} + T_{data\_hazard} + T_{control\_flush} + T_{memory\_stall} + T_{drain}$$

Where:
- $T_{productive}$: Cycles where instructions advance without structural, data, or control impediments.
- $T_{data\_hazard}$: Cycles stalled due to load-use hazards.
- $T_{control\_flush}$: Bubble cycles introduced into the pipeline following a branch misprediction or jump.
- $T_{memory\_stall}$: Wait cycles elapsed waiting for cache miss line fills or writebacks.
- $T_{drain}$: Residual cycles required to drain remaining in-flight instructions after `HALT` has been fetched.

### Pipeline Utilization

$$\text{Utilization} = \frac{T_{productive}}{T_{cycles}} \times 100\%$$

---

## 3. Cache Metrics

### Cache Hit Rate ($H_C$)

$$H_C = \frac{\text{Read Hits} + \text{Write Hits}}{\text{Total Cache Accesses}}$$

### Cache Miss Rate ($M_C$)

$$M_C = 1 - H_C = \frac{\text{Read Misses} + \text{Write Misses}}{\text{Total Cache Accesses}}$$

### Total Memory Traffic (Words)

$$\text{Traffic} = (\text{Read Misses} + \text{Write Misses}) \times \frac{\text{Line Size}}{4} + \text{Dirty Writebacks} \times \frac{\text{Line Size}}{4}$$

---

## 4. Workload Benchmark Measurements (Real Simulator Output)

Data collected from executions recorded in `results/workload_summary.csv`:

| Workload | Retired Instr | Total Cycles | CPI | IPC | Data Stalls | Branch Flushes | Cache Misses | Hit Rate |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| `01_independent_arithmetic` | 17 | 21 | 1.24 | 0.81 | 0 | 0 | 0 | N/A |
| `02_dependent_arithmetic` | 11 | 15 | 1.36 | 0.73 | 0 | 0 | 0 | N/A |
| `03_load_use_hazards` | 12 | 27 | 2.25 | 0.44 | 3 | 0 | 1 | 83.3% |
| `04_store_forwarding` | 11 | 23 | 2.09 | 0.48 | 0 | 0 | 1 | 83.3% |
| `05_branch_taken_not_taken` | 8 | 14 | 1.75 | 0.57 | 0 | 1 | 0 | N/A |
| `06_branch_heavy_loop` | 64 | 106 | 1.66 | 0.60 | 0 | 19 | 0 | N/A |
| `07_sequential_memory` | 184 | 296 | 1.61 | 0.62 | 16 | 30 | 4 | 87.5% |
| `08_strided_memory` | 53 | 135 | 2.55 | 0.39 | 0 | 7 | 8 | 50.0% |
| `09_small_working_set` | 83 | 133 | 1.60 | 0.62 | 20 | 9 | 1 | 97.5% |
| `10_large_working_set` | 351 | 625 | 1.78 | 0.56 | 48 | 47 | 16 | 83.3% |
| `11_matrix_mult` | 56 | 84 | 1.50 | 0.67 | 0 | 0 | 3 | 85.0% |

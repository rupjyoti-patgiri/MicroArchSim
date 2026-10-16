# Memory Hierarchy and Cache Subsystem

## 1. Subsystem Architecture

MicroArchSim models a parameterized L1 Data Cache integrated directly with the processor's `MEM` stage.

```
+---------------+     Virtual / Physical Addr     +-------------+
|               | ----------------------------->  |             |
| Pipeline MEM  |                                 |  L1 Data    |
|    Stage      | <-----------------------------  |   Cache     |
+---------------+     Data Word / Stall Signal    +-------------+
                                                         |
                                             Line Fill / | Miss / Eviction
                                             Writeback   v
                                                  +-------------+
                                                  | Main Memory |
                                                  +-------------+
```

---

## 2. Address Decomposition

Addresses are decomposed based on cache geometry:

$$\text{Address (32 bits)} = [\ \text{Tag}\ (32 - s - b)\ |\ \text{Index}\ (s)\ |\ \text{Offset}\ (b)\ ]$$

- Line size: $B$ bytes $\implies b = \log_2(B)$ offset bits.
- Associativity: $A$ ways.
- Number of Sets: $S = \frac{\text{Total Cache Size}}{B \times A} \implies s = \log_2(S)$ index bits.
- Tag: Remaining high-order bits.

---

## 3. Cache Policies and Features

1. **Configurable Associativity**:
   - $A = 1$: Direct-Mapped (fast lookup, subject to conflict misses).
   - $1 < A < \text{Lines}$: Set-Associative (balances lookup latency and hit rate).
   - $A = \text{Lines}$: Fully-Associative (minimizes conflict misses).
2. **Replacement Policies**:
   - **LRU (Least Recently Used)**: Monotonically incremented access counter tracks line age. The way with the minimum counter value in the set is selected as the eviction victim.
   - **FIFO (First-In, First-Out)**: Timestamp recorded strictly upon line allocation.
   - **Random**: Reproducible pseudo-random replacement via standard PRNG.
3. **Write and Allocation Policies**:
   - **Write-Back + Write-Allocate (Default)**: Writes update only the cache line and mark `dirty = true`. Main memory is only updated when a dirty line is evicted. Misses bring the target line into cache before writing.
   - **Write-Through + No-Write-Allocate**: Writes propagate immediately to backing memory; write misses bypass the cache without allocation.

---

## 4. Pipeline Integration and Stall Timing Model

### Blocking Cache Design
MicroArchSim implements a **blocking cache**:
- **Cache Hit**: Operation completes within configured `hit_latency` cycles (default: 1 cycle).
- **Cache Miss**: Introduces a multi-cycle stall (`miss_penalty - 1` wait cycles, default: 9 cycles).
- **Stall Mechanics**:
  - The pipeline freezes upstream execution during the memory stall.
  - The memory operation completes line allocation and victim writeback upon countdown completion.
  - Prevents side-effects or duplicate memory requests when execution resumes.

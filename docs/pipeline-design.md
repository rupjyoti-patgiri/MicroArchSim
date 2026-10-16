# Five-Stage Pipeline Microarchitecture

## 1. Classical 5-Stage RISC Pipeline

MicroArchSim implements the canonical in-order 5-stage pipeline:

```
[ IF ] ---> [ ID ] ---> [ EX ] ---> [ MEM ] ---> [ WB ]
Instruction  Instruction   ALU / Br     Data Mem      Register
   Fetch       Decode      Execution     Access       Writeback
```

### Stage Responsibilities
1. **Instruction Fetch (IF)**:
   - Accesses instruction memory at current PC.
   - Computes sequential next PC ($PC + 4$).
   - Latches instruction word and PC into `IF/ID`.
2. **Instruction Decode and Register Read (ID)**:
   - Decodes opcode, funct3, funct7, rs1, rs2, rd, and immediate.
   - Reads operands from the register file.
   - For unconditional jump (`JAL`): resolves target address in ID stage ($PC + imm$), redirects fetch path, and triggers 1-cycle jump flush.
3. **Execute and Address Calculation (EX)**:
   - ALU performs arithmetic, logical, or comparison operation.
   - Computes effective data memory address ($rs1 + imm$) for loads and stores.
   - Resolves conditional branches (`BEQ`, `BNE`). On taken branch, redirects PC to target and triggers 2-cycle branch flush.
4. **Data Memory Access (MEM)**:
   - Accesses L1 data cache / backing data memory.
   - On load (`LW`): receives read data (or triggers memory stall on cache miss).
   - On store (`SW`): writes forwarded $rs2$ data to cache.
5. **Write Back (WB)**:
   - Commits computation result (ALU result or memory load data) to register file at destination index $rd$.
   - Increments retirement counter.

---

## 2. Explicit Pipeline Registers

Pipeline stages communicate strictly through registered boundaries:

```
+----+  IF/ID  +----+  ID/EX  +----+  EX/MEM +----+  MEM/WB +----+
| IF | ======= | ID | ======= | EX | ======= | MEM| ======= | WB |
+----+         +----+         +----+         +----+         +----+
```

### Double-Buffered Cycle Semantics
To prevent intra-cycle write-after-read hazards in C++ simulation, MicroArchSim implements a **two-phase clock / double-buffered state model**:
- `current_`: Represents the pipeline state at the start of clock cycle $t$.
- `next_`: Accumulates updates computed during cycle $t$.
- **Reverse Stage Execution**: Stages are evaluated in order `WB` $\rightarrow$ `MEM` $\rightarrow$ `EX` $\rightarrow$ `ID` $\rightarrow$ `IF`. This guarantees downstream stages read values produced in the prior cycle.
- **Atomic State Commit**: At the cycle boundary, `current_ = next_` commits all updates simultaneously.

---

## 3. Pipeline Register Fields

| Register | Key Fields Stored |
|:---|:---|
| **`IF/ID`** | `instruction_word`, `pc`, `next_pc`, `valid` |
| **`ID/EX`** | `pc`, `id`, `rs1`, `rs2`, `rd`, `rs1_val`, `rs2_val`, `immediate`, `alu_op`, `reg_write`, `mem_read`, `mem_write`, `mem_to_reg`, `is_branch`, `is_jump`, `is_halt`, `valid` |
| **`EX/MEM`** | `pc`, `id`, `alu_result`, `rs2_val` (store data), `rd`, `reg_write`, `mem_read`, `mem_write`, `mem_to_reg`, `is_halt`, `branch_taken`, `branch_target`, `valid` |
| **`MEM/WB`** | `id`, `rd`, `alu_result`, `mem_data`, `reg_write`, `mem_to_reg`, `is_halt`, `valid` |

---

## 4. Pipeline Bubble and Flush Semantics

- **Bubble (NOP Injection)**: An invalid pipeline stage (`valid = false`). No architectural state is modified when a bubble traverses downstream stages.
- **Stall Insertion**: Freezes upstream stages (holds current values in `PC` and `IF/ID`) and clears `ID/EX` to insert a bubble.
- **Flush Mechanism**:
  - `branch_flush_`: Clears both `IF/ID` and `ID/EX`, updating $PC$ to the branch target (2 wasted slots).
  - `jump_flush_`: Clears `IF/ID`, updating $PC$ to the jump target (1 wasted slot).
- **Speculative Drain Reset**: If `HALT` was speculatively fetched down a fall-through branch path, the flush resets `halt_fetched_ = false`, allowing loop execution to continue without premature termination.

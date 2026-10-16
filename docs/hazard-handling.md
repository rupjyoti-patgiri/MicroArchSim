# Hazard Detection and Resolution Unit

## 1. Data Hazards (RAW Dependencies)

A Read-After-Write (RAW) hazard occurs when an instruction in `ID` or `EX` depends on the result of an older in-flight instruction that has not yet committed its value to the register file.

```
Cycle 1:  ADD  x1, x2, x3   [IF] [ID] [EX] [MEM] [WB] (x1 written at WB end)
Cycle 2:  SUB  x4, x1, x5        [IF] [ID]  [EX]  [MEM] [WB] (x1 needed at EX start)
```

Without forwarding, `SUB` would have to stall until `ADD` completes `WB` (a 2-cycle penalty).

---

## 2. Forwarding Unit Architecture

MicroArchSim incorporates full **operand forwarding (bypassing)**:

### Forwarding Paths
1. **EX/MEM $\rightarrow$ EX**:
   - Forwards result computed by the immediately preceding instruction in `EX/MEM` to the ALU input of the instruction in `ID/EX`.
   - Condition: `EX/MEM.valid && EX/MEM.reg_write && EX/MEM.rd != 0 && EX/MEM.rd == rs`
2. **MEM/WB $\rightarrow$ EX**:
   - Forwards result computed two instructions earlier in `MEM/WB` to the ALU input of the instruction in `ID/EX`.
   - Condition: `MEM/WB.valid && MEM/WB.reg_write && MEM/WB.rd != 0 && MEM/WB.rd == rs`

### Strict Forwarding Priority
If both `EX/MEM` and `MEM/WB` write to the same destination register ($rd_{EX/MEM} == rd_{MEM/WB} == rs$), the forwarding unit **MUST prioritize `EX/MEM`**. The instruction in `EX/MEM` is chronologically younger and holds the most recent architectural value.

### Store-Data Forwarding
Stores require two operands:
1. Base address register ($rs1$): Forwarded into ALU operand A.
2. Store data register ($rs2$): Forwarded into `EX/MEM.rs2_val`.
MicroArchSim forwards directly into store data operands, allowing an immediately preceding calculation to be stored to memory without any stall cycles.

---

## 3. Load-Use Data Hazards

A load instruction (`LW`) does not obtain data from the cache until the end of the `MEM` stage. Therefore, when an immediately following instruction requires the loaded value in `EX`, forwarding cannot travel backward in time:

```
Cycle:         1    2    3    4    5    6
LW  x1, 0(x2) [IF] [ID] [EX] [MEM] [WB]
                              \---> Data available here
ADD x3, x1, x4     [IF] [ID]  [EX] ----> Data needed here! (Impossible)
```

### Hardware Stall Mechanics
When `ID/EX.mem_read && ID/EX.rd != 0 && (ID/EX.rd == rs1 || ID/EX.rd == rs2)`:
1. **Freeze `PC`**: Holds current value for the next cycle.
2. **Freeze `IF/ID`**: Preserves the dependent instruction in decode.
3. **Inject Bubble into `ID/EX`**: Inserts a cleared NOP stage into execute.
4. Next cycle, the load advances to `MEM/WB`, allowing `MEM/WB $\rightarrow$ EX` forwarding to supply the loaded value to the ALU.

---

## 4. Control Hazards (Branches and Jumps)

### Branch Policy
- **Static Predict Not-Taken**: The fetch unit sequentially fetches $PC + 4$.
- **Conditional Branches (`BEQ`, `BNE`)**: Resolved in `EX` stage.
  - If condition is met (taken branch), $PC$ is redirected to $PC + imm$.
  - Two instructions fetched in speculative error (`IF/ID` and `ID/EX`) are squashed (`branch_flush_ = true`).
  - Penalty: Exactly **2 cycles**.
- **Unconditional Jumps (`JAL`)**:
  - Target computed and resolved in `ID` stage.
  - Only one speculative instruction (`IF/ID`) is squashed (`jump_flush_ = true`).
  - Penalty: Exactly **1 cycle**.

# MicroArchSim: Architectural Specification

## 1. Overview and Scope

**MicroArchSim** models a single-issue, in-order, 32-bit RISC-V-like processor implementing the **RV32I-Edu** instruction subset. It is designed to expose cycle-by-cycle pipeline dynamics, register file behavior, hazard resolution mechanics, and memory hierarchy interactions without the confounding complexities of full OS virtualization, privilege modes, or external device buses.

### Architectural vs Microarchitectural State
- **Architectural State**: State defined by the programmer visible ISA:
  - 32 general-purpose 32-bit registers ($x_0$ to $x_{31}$), where $x_0 \equiv 0$ unconditionally.
  - Program Counter (PC), word-aligned ($PC[1:0] = 00$).
  - Byte-addressable uniform 32-bit memory space.
- **Microarchitectural State**: State internal to the processor implementation that affects performance (cycles) but never architectural correctness:
  - Pipeline registers (`IF/ID`, `ID/EX`, `EX/MEM`, `MEM/WB`).
  - Forwarding paths and multiplexer controls.
  - Hazard detection unit status and pipeline stall/flush latches.
  - L1 Cache lines, tags, valid bits, dirty bits, and LRU access counters.

---

## 2. Supported Instruction Set Architecture (RV32I-Edu)

MicroArchSim supports 17 instructions spanning arithmetic, logical, memory, and control transfer operations:

| Mnemonic | Format | Opcode (`[6:0]`) | funct3 (`[14:12]`) | funct7 (`[31:25]`) | Description |
|:---|:---|:---:|:---:|:---:|:---|
| **ADD** | R | `0110011` | `000` | `0000000` | $R[rd] \leftarrow R[rs1] + R[rs2]$ |
| **SUB** | R | `0110011` | `000` | `0100000` | $R[rd] \leftarrow R[rs1] - R[rs2]$ |
| **AND** | R | `0110011` | `111` | `0000000` | $R[rd] \leftarrow R[rs1] \ \& \ R[rs2]$ |
| **OR** | R | `0110011` | `110` | `0000000` | $R[rd] \leftarrow R[rs1] \ \| \ R[rs2]$ |
| **XOR** | R | `0110011` | `100` | `0000000` | $R[rd] \leftarrow R[rs1] \oplus R[rs2]$ |
| **SLT** | R | `0110011` | `010` | `0000000` | $R[rd] \leftarrow (R[rs1]_{signed} < R[rs2]_{signed}) \ ? \ 1 : 0$ |
| **ADDI** | I | `0010011` | `000` | N/A | $R[rd] \leftarrow R[rs1] + \text{sign\_ext}(imm[11:0])$ |
| **ANDI** | I | `0010011` | `111` | N/A | $R[rd] \leftarrow R[rs1] \ \& \ \text{sign\_ext}(imm[11:0])$ |
| **SLTI** | I | `0010011` | `010` | N/A | $R[rd] \leftarrow (R[rs1]_{signed} < imm_{signed}) \ ? \ 1 : 0$ |
| **LUI** | U | `0110111` | N/A | N/A | $R[rd] \leftarrow imm[31:12] \ll 12$ |
| **AUIPC** | U | `0010111` | N/A | N/A | $R[rd] \leftarrow PC + (imm[31:12] \ll 12)$ |
| **LW** | I | `0000011` | `010` | N/A | $R[rd] \leftarrow M_{32}[R[rs1] + \text{sign\_ext}(imm)]$ |
| **SW** | S | `0100011` | `010` | N/A | $M_{32}[R[rs1] + \text{sign\_ext}(imm)] \leftarrow R[rs2]$ |
| **BEQ** | B | `1100011` | `000` | N/A | If $R[rs1] == R[rs2]$: $PC \leftarrow PC + imm$ |
| **BNE** | B | `1100011` | `001` | N/A | If $R[rs1] \neq R[rs2]$: $PC \leftarrow PC + imm$ |
| **JAL** | J | `1101111` | N/A | N/A | $R[rd] \leftarrow PC + 4; \ PC \leftarrow PC + imm$ |
| **HALT** | System | `1110011` | N/A | N/A | Halts simulator execution (encoded as `EBREAK`) |

### Register File Rules
1. **$x_0$ Hardwired Zero**:
   - Reads from register index `0` unconditionally return `0x00000000`.
   - Writes directed to register index `0` are ignored at commit time without raising an exception.
2. **Registers $x_1$–$x_{31}$**: General purpose registers preserving written 32-bit values across cycles.

---

## 3. Dual Execution Models: Reference vs. Pipeline

To guarantee absolute verification and zero architectural regressions, MicroArchSim implements two separate simulation models:

1. **Sequential Reference Interpreter (`ReferenceInterpreter`)**:
   - Executes instructions in atomic sequence: fetch $\rightarrow$ decode $\rightarrow$ execute $\rightarrow$ memory $\rightarrow$ writeback completes fully before next $PC$ increments.
   - Zero hazards, zero stalls, zero pipeline bubbles.
   - Serves as the golden verification oracle.
2. **Cycle-Accurate Five-Stage Pipeline (`Pipeline`)**:
   - Discrete cycle-stepped engine advancing five instructions simultaneously.
   - Explicit inter-stage pipeline registers.
   - Integrated hazard unit and cache memory latency stalls.

---

## 4. Differences from Commercial Microarchitectures

1. **In-Order vs Out-of-Order (OoO)**: MicroArchSim executes strictly in program order. Modern high-performance cores (e.g. NVIDIA Denver/Carmel, Intel Golden Cove) use dynamic instruction scheduling, register renaming (RAT), and reorder buffers (ROB).
2. **Branch Prediction**: Uses static branch prediction (predict not-taken). Modern cores employ TAGE, branch target buffers (BTB), and return address stacks (RAS).
3. **Memory Consistency**: Sequential consistency is maintained. MicroArchSim does not model store buffers, invalidation queues, or relaxed memory models (e.g., RVWMO or TSO).

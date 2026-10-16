#pragma once

/// @file pipeline_registers.h
/// @brief Pipeline register structs for the five-stage processor.
///
/// Each struct represents the latch between two pipeline stages.
/// Fields carry instruction data, control signals, and validity state
/// from one stage to the next. A double-buffered scheme prevents
/// read-after-write races within a single cycle.

#include "types.h"

namespace microarchsim {

// ============================================================================
// IF/ID Pipeline Register
// ============================================================================

struct IFID_Register {
    Word instruction_word = NOP_INSTRUCTION;  ///< Raw 32-bit instruction
    Addr pc       = 0;       ///< PC of this instruction
    Addr next_pc  = 0;       ///< PC + 4 (predicted next PC)
    bool valid    = false;   ///< Is this a real instruction (not a bubble)?

    /// Insert a bubble (invalidate this register).
    void clear() {
        instruction_word = NOP_INSTRUCTION;
        pc = 0;
        next_pc = 0;
        valid = false;
    }
};

// ============================================================================
// ID/EX Pipeline Register
// ============================================================================

struct IDEX_Register {
    // Instruction identity
    Addr pc               = 0;
    InstructionID id      = InstructionID::NOP;

    // Register indices (for hazard detection)
    Reg rs1               = 0;
    Reg rs2               = 0;
    Reg rd                = 0;

    // Operand values (read from register file, before forwarding)
    Word rs1_val          = 0;
    Word rs2_val          = 0;

    // Immediate value (sign-extended)
    SWord immediate       = 0;

    // Control signals
    ALUOp alu_op          = ALUOp::NOP;
    bool reg_write        = false;   ///< Write to rd in WB?
    bool mem_read         = false;   ///< Read from memory in MEM?
    bool mem_write        = false;   ///< Write to memory in MEM?
    bool mem_to_reg       = false;   ///< Writeback data comes from memory?
    bool is_branch        = false;   ///< Conditional branch?
    bool is_jump          = false;   ///< Unconditional jump (JAL)?
    bool is_halt          = false;   ///< HALT instruction?

    bool valid            = false;

    void clear() {
        pc = 0;
        id = InstructionID::NOP;
        rs1 = rs2 = rd = 0;
        rs1_val = rs2_val = 0;
        immediate = 0;
        alu_op = ALUOp::NOP;
        reg_write = mem_read = mem_write = mem_to_reg = false;
        is_branch = is_jump = is_halt = false;
        valid = false;
    }
};

// ============================================================================
// EX/MEM Pipeline Register
// ============================================================================

struct EXMEM_Register {
    Addr pc               = 0;
    InstructionID id      = InstructionID::NOP;

    // Computed values
    Word alu_result       = 0;       ///< ALU output / computed address
    Word rs2_val          = 0;       ///< Store data (value of rs2, possibly forwarded)

    // Destination register
    Reg rd                = 0;

    // Control signals
    bool reg_write        = false;
    bool mem_read         = false;
    bool mem_write        = false;
    bool mem_to_reg       = false;
    bool is_halt          = false;

    // Branch resolution
    bool branch_taken     = false;   ///< Was the branch taken?
    Addr branch_target    = 0;       ///< Target address if taken

    bool valid            = false;

    void clear() {
        pc = 0;
        id = InstructionID::NOP;
        alu_result = rs2_val = 0;
        rd = 0;
        reg_write = mem_read = mem_write = mem_to_reg = is_halt = false;
        branch_taken = false;
        branch_target = 0;
        valid = false;
    }
};

// ============================================================================
// MEM/WB Pipeline Register
// ============================================================================

struct MEMWB_Register {
    InstructionID id      = InstructionID::NOP;

    // Data for writeback
    Word alu_result       = 0;       ///< ALU result (used if !mem_to_reg)
    Word mem_data         = 0;       ///< Memory read data (used if mem_to_reg)

    // Destination
    Reg rd                = 0;

    // Control signals
    bool reg_write        = false;
    bool mem_to_reg       = false;
    bool is_halt          = false;

    bool valid            = false;

    /// The value to be written back to the register file.
    Word write_data() const {
        return mem_to_reg ? mem_data : alu_result;
    }

    void clear() {
        id = InstructionID::NOP;
        alu_result = mem_data = 0;
        rd = 0;
        reg_write = mem_to_reg = is_halt = false;
        valid = false;
    }
};

// ============================================================================
// Aggregate pipeline state (for double-buffering)
// ============================================================================

struct PipelineState {
    IFID_Register  if_id;
    IDEX_Register  id_ex;
    EXMEM_Register ex_mem;
    MEMWB_Register mem_wb;
    Addr pc = INSTRUCTION_BASE;  ///< Current program counter

    void clear() {
        if_id.clear();
        id_ex.clear();
        ex_mem.clear();
        mem_wb.clear();
        pc = INSTRUCTION_BASE;
    }
};

} // namespace microarchsim

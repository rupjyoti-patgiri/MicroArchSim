#pragma once

/// @file reference_interpreter.h
/// @brief Sequential single-instruction reference interpreter (correctness oracle).

#include "types.h"
#include "instruction.h"
#include "register_file.h"
#include "memory.h"
#include <vector>

namespace microarchsim {

/// Sequential reference interpreter — executes one instruction at a time.
/// Used as a correctness oracle for validating pipelined execution.
class ReferenceInterpreter {
public:
    /// Construct with memory size.
    explicit ReferenceInterpreter(uint32_t memory_size = 1 << 20);

    /// Load instruction words into instruction memory.
    void load_program(const std::vector<Word>& instructions, Addr base = INSTRUCTION_BASE);

    /// Load data words into data memory.
    void load_data(const std::vector<Word>& data, Addr base = DATA_BASE);

    /// Run until HALT or max_instructions reached.
    /// @return Number of instructions executed.
    uint64_t run(uint64_t max_instructions = 1000000);

    /// Execute a single instruction. Returns false if HALT.
    bool step();

    /// Get the register file.
    const RegisterFile& registers() const { return reg_file_; }
    RegisterFile& registers() { return reg_file_; }

    /// Get the memory.
    const Memory& memory() const { return memory_; }
    Memory& memory() { return memory_; }

    /// Get current PC.
    Addr pc() const { return pc_; }

    /// Get instruction count.
    uint64_t instruction_count() const { return instruction_count_; }

    /// Has the interpreter halted?
    bool halted() const { return halted_; }

    /// Reset state (registers, memory, PC).
    void reset();

private:
    RegisterFile reg_file_;
    Memory memory_;
    Addr pc_ = INSTRUCTION_BASE;
    uint64_t instruction_count_ = 0;
    bool halted_ = false;
};

} // namespace microarchsim

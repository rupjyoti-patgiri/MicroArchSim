/// @file reference_interpreter.cpp
/// @brief Sequential reference interpreter implementation.

#include "reference_interpreter.h"
#include <sstream>

namespace microarchsim {

ReferenceInterpreter::ReferenceInterpreter(uint32_t memory_size)
    : memory_(memory_size) {}

void ReferenceInterpreter::load_program(const std::vector<Word>& instructions, Addr base) {
    memory_.load_program(instructions, base);
}

void ReferenceInterpreter::load_data(const std::vector<Word>& data, Addr base) {
    memory_.load_data(data, base);
}

uint64_t ReferenceInterpreter::run(uint64_t max_instructions) {
    while (!halted_ && instruction_count_ < max_instructions) {
        if (!step()) break;
    }
    return instruction_count_;
}

bool ReferenceInterpreter::step() {
    if (halted_) return false;

    // Fetch
    Word instruction_word = memory_.read_word(pc_);

    // Decode
    DecodedInstruction instr = decode(instruction_word);

    // Check for HALT
    if (instr.is_halt) {
        halted_ = true;
        return false;
    }

    instruction_count_++;

    // Read source registers
    Word rs1_val = reg_file_.read(instr.rs1);
    Word rs2_val = reg_file_.read(instr.rs2);
    SWord rs1_signed = static_cast<SWord>(rs1_val);
    SWord rs2_signed = static_cast<SWord>(rs2_val);

    // Default next PC
    Addr next_pc = pc_ + 4;

    // Execute
    Word result = 0;
    Word mem_data = 0;

    switch (instr.id) {
        // R-type arithmetic/logic
        case InstructionID::ADD:
            result = rs1_val + rs2_val;
            break;
        case InstructionID::SUB:
            result = rs1_val - rs2_val;
            break;
        case InstructionID::AND:
            result = rs1_val & rs2_val;
            break;
        case InstructionID::OR:
            result = rs1_val | rs2_val;
            break;
        case InstructionID::XOR:
            result = rs1_val ^ rs2_val;
            break;
        case InstructionID::SLT:
            result = (rs1_signed < rs2_signed) ? 1 : 0;
            break;

        // I-type arithmetic/logic
        case InstructionID::ADDI:
            result = rs1_val + static_cast<Word>(instr.immediate);
            break;
        case InstructionID::ANDI:
            result = rs1_val & static_cast<Word>(instr.immediate);
            break;
        case InstructionID::SLTI:
            result = (rs1_signed < instr.immediate) ? 1 : 0;
            break;

        // U-type
        case InstructionID::LUI:
            result = static_cast<Word>(instr.immediate);
            break;
        case InstructionID::AUIPC:
            result = pc_ + static_cast<Word>(instr.immediate);
            break;

        // Load
        case InstructionID::LW: {
            Addr addr = rs1_val + static_cast<Word>(instr.immediate);
            mem_data = memory_.read_word(addr);
            result = mem_data;
            break;
        }

        // Store
        case InstructionID::SW: {
            Addr addr = rs1_val + static_cast<Word>(instr.immediate);
            memory_.write_word(addr, rs2_val);
            break;
        }

        // Branches
        case InstructionID::BEQ:
            if (rs1_val == rs2_val) {
                next_pc = pc_ + static_cast<Word>(instr.immediate);
            }
            break;
        case InstructionID::BNE:
            if (rs1_val != rs2_val) {
                next_pc = pc_ + static_cast<Word>(instr.immediate);
            }
            break;

        // Jump
        case InstructionID::JAL:
            result = pc_ + 4;  // Link address
            next_pc = pc_ + static_cast<Word>(instr.immediate);
            break;

        case InstructionID::NOP:
            // Do nothing
            break;

        case InstructionID::HALT:
        case InstructionID::UNKNOWN:
            // Should not reach here — HALT handled above
            break;
    }

    // Writeback
    if (instr.reg_write && instr.rd != 0) {
        reg_file_.write(instr.rd, result);
    }

    // Update PC
    pc_ = next_pc;

    return true;
}

void ReferenceInterpreter::reset() {
    reg_file_.reset();
    memory_.reset();
    pc_ = INSTRUCTION_BASE;
    instruction_count_ = 0;
    halted_ = false;
}

} // namespace microarchsim

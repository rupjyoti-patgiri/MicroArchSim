#pragma once

/// @file instruction.h
/// @brief Instruction decoder, disassembler, and encoder definitions for RV32I-Edu.

#include "types.h"
#include <string>

namespace microarchsim {

/// Fully decoded instruction with all fields extracted.
struct DecodedInstruction {
    Word raw = 0;                 // Original 32-bit instruction word
    InstructionID id = InstructionID::UNKNOWN;         // Identified instruction (ADD, SUB, etc.)
    InstructionFormat format = InstructionFormat::UNKNOWN; // R, I, S, B, U, J

    Reg rd = 0;                   // Destination register (0-31)
    Reg rs1 = 0;                  // Source register 1 (0-31)
    Reg rs2 = 0;                  // Source register 2 (0-31)

    SWord immediate = 0;          // Sign-extended immediate value

    uint8_t funct3 = 0;           // funct3 field
    uint8_t funct7 = 0;           // funct7 field

    // Control signals derived from decode
    ALUOp alu_op = ALUOp::NOP;    // ALU operation to perform
    bool reg_write = false;       // Does this instruction write to rd?
    bool mem_read = false;        // Does this instruction read from memory?
    bool mem_write = false;       // Does this instruction write to memory?
    bool is_branch = false;       // Is this a conditional branch?
    bool is_jump = false;         // Is this a jump (JAL)?
    bool mem_to_reg = false;      // Does writeback come from memory (LW)?
    bool is_halt = false;         // Is this a HALT instruction?
};

/// Decode a 32-bit instruction word into a DecodedInstruction.
/// @throws DecodeError on invalid or unsupported instruction words.
DecodedInstruction decode(Word instruction_word);

/// Return human-readable disassembly of a decoded instruction.
std::string disassemble(const DecodedInstruction& instr);

/// Encode an R-type instruction from its fields.
Word encode_r_type(uint8_t funct7, Reg rs2, Reg rs1, uint8_t funct3, Reg rd, uint8_t opcode);

/// Encode an I-type instruction.
Word encode_i_type(SWord imm, Reg rs1, uint8_t funct3, Reg rd, uint8_t opcode);

/// Encode an S-type instruction.
Word encode_s_type(SWord imm, Reg rs2, Reg rs1, uint8_t funct3, uint8_t opcode);

/// Encode a B-type instruction.
Word encode_b_type(SWord imm, Reg rs2, Reg rs1, uint8_t funct3, uint8_t opcode);

/// Encode a U-type instruction.
Word encode_u_type(SWord imm, Reg rd, uint8_t opcode);

/// Encode a J-type instruction.
Word encode_j_type(SWord imm, Reg rd, uint8_t opcode);

} // namespace microarchsim

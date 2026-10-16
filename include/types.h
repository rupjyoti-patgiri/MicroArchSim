#pragma once

/// @file types.h
/// @brief Core type definitions, opcodes, and enumerations for MicroArchSim.
///
/// This file defines the fundamental types used throughout the simulator,
/// including the RV32I-Edu instruction set encoding constants.

#include <cstdint>
#include <string>
#include <stdexcept>

namespace microarchsim {

// ============================================================================
// Fundamental type aliases
// ============================================================================

using Word  = uint32_t;   ///< 32-bit unsigned word (registers, memory data)
using SWord = int32_t;    ///< 32-bit signed word (sign-extended immediates)
using Addr  = uint32_t;   ///< 32-bit memory address
using Reg   = uint8_t;    ///< Register index (0–31)

// ============================================================================
// Constants
// ============================================================================

constexpr int NUM_REGISTERS = 32;
constexpr Addr INSTRUCTION_BASE = 0x00000000;  ///< Default instruction memory base
constexpr Addr DATA_BASE        = 0x00010000;  ///< Default data memory base
constexpr Word NOP_INSTRUCTION  = 0x00000013;  ///< ADDI x0, x0, 0
constexpr Word HALT_INSTRUCTION = 0x00100073;  ///< EBREAK — used as HALT sentinel

// ============================================================================
// RV32I Opcodes (bits [6:0])
// ============================================================================

enum class OpcodeType : uint8_t {
    R_TYPE    = 0b0110011,  // ADD, SUB, AND, OR, XOR, SLT
    I_TYPE    = 0b0010011,  // ADDI, ANDI, SLTI
    LOAD      = 0b0000011,  // LW
    STORE     = 0b0100011,  // SW
    BRANCH    = 0b1100011,  // BEQ, BNE
    JAL       = 0b1101111,  // JAL
    LUI       = 0b0110111,  // LUI
    AUIPC     = 0b0010111,  // AUIPC
    SYSTEM    = 0b1110011,  // EBREAK (HALT)
};

// ============================================================================
// Funct3 fields
// ============================================================================

namespace funct3 {
    // R-type and I-type arithmetic/logic
    constexpr uint8_t ADD_SUB  = 0b000;
    constexpr uint8_t SLT      = 0b010;
    constexpr uint8_t XOR      = 0b100;
    constexpr uint8_t OR       = 0b110;
    constexpr uint8_t AND      = 0b111;

    // Load/Store
    constexpr uint8_t WORD     = 0b010;

    // Branch
    constexpr uint8_t BEQ      = 0b000;
    constexpr uint8_t BNE      = 0b001;
}

// ============================================================================
// Funct7 fields
// ============================================================================

namespace funct7 {
    constexpr uint8_t NORMAL   = 0b0000000;  // ADD, AND, OR, XOR, SLT
    constexpr uint8_t ALT      = 0b0100000;  // SUB
}

// ============================================================================
// Instruction format type
// ============================================================================

enum class InstructionFormat {
    R_TYPE,
    I_TYPE,
    S_TYPE,
    B_TYPE,
    U_TYPE,
    J_TYPE,
    UNKNOWN
};

// ============================================================================
// ALU operation
// ============================================================================

enum class ALUOp {
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLT,       ///< Set Less Than (signed)
    PASS_B,    ///< Pass operand B through (for LUI)
    ADD_PC,    ///< Add to PC (for AUIPC — handled specially)
    NOP        ///< No operation
};

// ============================================================================
// Instruction identity (fully decoded instruction type)
// ============================================================================

enum class InstructionID {
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLT,
    ADDI,
    ANDI,
    SLTI,
    LUI,
    AUIPC,
    LW,
    SW,
    BEQ,
    BNE,
    JAL,
    NOP,       ///< Encoded as ADDI x0, x0, 0
    HALT,      ///< Encoded as EBREAK
    UNKNOWN
};

/// Return human-readable name for an instruction.
inline std::string instruction_name(InstructionID id) {
    switch (id) {
        case InstructionID::ADD:     return "ADD";
        case InstructionID::SUB:     return "SUB";
        case InstructionID::AND:     return "AND";
        case InstructionID::OR:      return "OR";
        case InstructionID::XOR:     return "XOR";
        case InstructionID::SLT:     return "SLT";
        case InstructionID::ADDI:    return "ADDI";
        case InstructionID::ANDI:    return "ANDI";
        case InstructionID::SLTI:    return "SLTI";
        case InstructionID::LUI:     return "LUI";
        case InstructionID::AUIPC:   return "AUIPC";
        case InstructionID::LW:      return "LW";
        case InstructionID::SW:      return "SW";
        case InstructionID::BEQ:     return "BEQ";
        case InstructionID::BNE:     return "BNE";
        case InstructionID::JAL:     return "JAL";
        case InstructionID::NOP:     return "NOP";
        case InstructionID::HALT:    return "HALT";
        case InstructionID::UNKNOWN: return "UNKNOWN";
    }
    return "???";
}

// ============================================================================
// Forwarding source
// ============================================================================

enum class ForwardSource {
    NONE,       ///< No forwarding, use register file value
    EX_MEM,     ///< Forward from EX/MEM pipeline register
    MEM_WB      ///< Forward from MEM/WB pipeline register
};

// ============================================================================
// Cache-related enums
// ============================================================================

enum class ReplacementPolicy {
    LRU,
    FIFO,
    RANDOM
};

enum class WritePolicy {
    WRITE_BACK,
    WRITE_THROUGH
};

enum class AllocatePolicy {
    WRITE_ALLOCATE,
    NO_WRITE_ALLOCATE
};

// ============================================================================
// Cycle classification for statistics
// ============================================================================

enum class CycleType {
    PRODUCTIVE,         ///< At least one instruction advanced
    DATA_HAZARD_STALL,  ///< Stall due to load-use hazard
    CONTROL_FLUSH,      ///< Cycle where flushed instructions occupy slots
    MEMORY_STALL,       ///< Stall waiting for cache miss
    DRAIN               ///< Pipeline draining after HALT
};

// ============================================================================
// Simulator mode
// ============================================================================

enum class SimMode {
    REFERENCE,   ///< Sequential single-instruction interpreter
    PIPELINE     ///< Five-stage pipelined execution
};

// ============================================================================
// Exception types
// ============================================================================

class SimulatorError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DecodeError : public SimulatorError {
public:
    using SimulatorError::SimulatorError;
};

class MemoryError : public SimulatorError {
public:
    using SimulatorError::SimulatorError;
};

class AssemblyError : public SimulatorError {
public:
    using SimulatorError::SimulatorError;
};

} // namespace microarchsim

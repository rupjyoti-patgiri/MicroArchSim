/// @file instruction.cpp
/// @brief Instruction decoder, disassembler, and encoder implementation for RV32I-Edu.

#include "instruction.h"
#include <iomanip>
#include <sstream>

namespace microarchsim {

namespace {

// ============================================================================
// Bit extraction helpers
// ============================================================================

inline uint8_t extract_opcode(Word inst) {
    return static_cast<uint8_t>(inst & 0x7FU);
}

inline Reg extract_rd(Word inst) {
    return static_cast<Reg>((inst >> 7) & 0x1FU);
}

inline uint8_t extract_funct3(Word inst) {
    return static_cast<uint8_t>((inst >> 12) & 0x07U);
}

inline Reg extract_rs1(Word inst) {
    return static_cast<Reg>((inst >> 15) & 0x1FU);
}

inline Reg extract_rs2(Word inst) {
    return static_cast<Reg>((inst >> 20) & 0x1FU);
}

inline uint8_t extract_funct7(Word inst) {
    return static_cast<uint8_t>((inst >> 25) & 0x7FU);
}

// ============================================================================
// Immediate extraction helpers (with correct sign extension)
// ============================================================================

// I-type: inst[31:20], sign-extended from bit 11
inline SWord extract_imm_i(Word inst) {
    uint32_t imm = (inst >> 20) & 0xFFFU;
    if (imm & (1U << 11)) {
        imm |= 0xFFFFF000U;
    }
    return static_cast<SWord>(imm);
}

// S-type: {inst[31:25], inst[11:7]}, sign-extended from bit 11
inline SWord extract_imm_s(Word inst) {
    uint32_t imm = (((inst >> 25) & 0x7FU) << 5) |
                   ((inst >> 7) & 0x1FU);
    if (imm & (1U << 11)) {
        imm |= 0xFFFFF000U;
    }
    return static_cast<SWord>(imm);
}

// B-type: {inst[31], inst[7], inst[30:25], inst[11:8], 0}, sign-extended from bit 12
inline SWord extract_imm_b(Word inst) {
    uint32_t imm = (((inst >> 31) & 0x01U) << 12) |
                   (((inst >> 7)  & 0x01U) << 11) |
                   (((inst >> 25) & 0x3FU) << 5)  |
                   (((inst >> 8)  & 0x0FU) << 1);
    if (imm & (1U << 12)) {
        imm |= 0xFFFFE000U;
    }
    return static_cast<SWord>(imm);
}

// U-type: {inst[31:12], 12'b0} — the upper 20 bits shifted left by 12
inline SWord extract_imm_u(Word inst) {
    return static_cast<SWord>(inst & 0xFFFFF000U);
}

// J-type: {inst[31], inst[19:12], inst[20], inst[30:21], 0}, sign-extended from bit 20
inline SWord extract_imm_j(Word inst) {
    uint32_t imm = (((inst >> 31) & 0x01U)  << 20) |
                   (((inst >> 12) & 0xFFU)  << 12) |
                   (((inst >> 20) & 0x01U)  << 11) |
                   (((inst >> 21) & 0x3FFU) << 1);
    if (imm & (1U << 20)) {
        imm |= 0xFFE00000U;
    }
    return static_cast<SWord>(imm);
}

} // anonymous namespace

// ============================================================================
// Instruction Decoder
// ============================================================================

DecodedInstruction decode(Word instruction_word) {
    DecodedInstruction instr;
    instr.raw = instruction_word;

    const uint8_t opcode = extract_opcode(instruction_word);
    const Reg raw_rd = extract_rd(instruction_word);
    const uint8_t raw_funct3 = extract_funct3(instruction_word);
    const Reg raw_rs1 = extract_rs1(instruction_word);
    const Reg raw_rs2 = extract_rs2(instruction_word);
    const uint8_t raw_funct7 = extract_funct7(instruction_word);

    switch (static_cast<OpcodeType>(opcode)) {
        case OpcodeType::R_TYPE: {
            instr.format = InstructionFormat::R_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = raw_rs1;
            instr.rs2 = raw_rs2;
            instr.funct3 = raw_funct3;
            instr.funct7 = raw_funct7;
            instr.immediate = 0;
            instr.reg_write = true;
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;

            if (raw_funct7 == funct7::NORMAL) {
                switch (raw_funct3) {
                    case funct3::ADD_SUB:
                        instr.id = InstructionID::ADD;
                        instr.alu_op = ALUOp::ADD;
                        break;
                    case funct3::AND:
                        instr.id = InstructionID::AND;
                        instr.alu_op = ALUOp::AND;
                        break;
                    case funct3::OR:
                        instr.id = InstructionID::OR;
                        instr.alu_op = ALUOp::OR;
                        break;
                    case funct3::XOR:
                        instr.id = InstructionID::XOR;
                        instr.alu_op = ALUOp::XOR;
                        break;
                    case funct3::SLT:
                        instr.id = InstructionID::SLT;
                        instr.alu_op = ALUOp::SLT;
                        break;
                    default: {
                        instr.id = InstructionID::UNKNOWN;
                        std::stringstream ss;
                        ss << "Unknown funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                           << ") for R-type instruction: 0x" << std::setw(8) << std::setfill('0')
                           << instruction_word;
                        throw DecodeError(ss.str());
                    }
                }
            } else if (raw_funct7 == funct7::ALT) {
                if (raw_funct3 == funct3::ADD_SUB) {
                    instr.id = InstructionID::SUB;
                    instr.alu_op = ALUOp::SUB;
                } else {
                    instr.id = InstructionID::UNKNOWN;
                    std::stringstream ss;
                    ss << "Unknown funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                       << ") with funct7::ALT for R-type instruction: 0x"
                       << std::setw(8) << std::setfill('0') << instruction_word;
                    throw DecodeError(ss.str());
                }
            } else {
                instr.id = InstructionID::UNKNOWN;
                std::stringstream ss;
                ss << "Unknown funct7 (0x" << std::hex << static_cast<int>(raw_funct7)
                   << ") for R-type instruction: 0x" << std::setw(8) << std::setfill('0')
                   << instruction_word;
                throw DecodeError(ss.str());
            }
            break;
        }

        case OpcodeType::I_TYPE: {
            instr.format = InstructionFormat::I_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = raw_rs1;
            instr.rs2 = 0;
            instr.funct3 = raw_funct3;
            instr.funct7 = 0;
            instr.immediate = extract_imm_i(instruction_word);
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;

            switch (raw_funct3) {
                case funct3::ADD_SUB:
                    if (raw_rd == 0 && raw_rs1 == 0 && instr.immediate == 0) {
                        instr.id = InstructionID::NOP;
                        instr.alu_op = ALUOp::NOP;
                        instr.reg_write = false;
                    } else {
                        instr.id = InstructionID::ADDI;
                        instr.alu_op = ALUOp::ADD;
                        instr.reg_write = true;
                    }
                    break;
                case funct3::AND:
                    instr.id = InstructionID::ANDI;
                    instr.alu_op = ALUOp::AND;
                    instr.reg_write = true;
                    break;
                case funct3::SLT:
                    instr.id = InstructionID::SLTI;
                    instr.alu_op = ALUOp::SLT;
                    instr.reg_write = true;
                    break;
                default: {
                    instr.id = InstructionID::UNKNOWN;
                    std::stringstream ss;
                    ss << "Unknown funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                       << ") for I-type instruction: 0x" << std::setw(8) << std::setfill('0')
                       << instruction_word;
                    throw DecodeError(ss.str());
                }
            }
            break;
        }

        case OpcodeType::LOAD: {
            instr.format = InstructionFormat::I_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = raw_rs1;
            instr.rs2 = 0;
            instr.funct3 = raw_funct3;
            instr.funct7 = 0;
            instr.immediate = extract_imm_i(instruction_word);
            instr.reg_write = true;
            instr.mem_read = true;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = true;
            instr.is_halt = false;

            if (raw_funct3 == funct3::WORD) {
                instr.id = InstructionID::LW;
                instr.alu_op = ALUOp::ADD;
            } else {
                instr.id = InstructionID::UNKNOWN;
                std::stringstream ss;
                ss << "Unsupported LOAD funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                   << "): 0x" << std::setw(8) << std::setfill('0') << instruction_word;
                throw DecodeError(ss.str());
            }
            break;
        }

        case OpcodeType::STORE: {
            instr.format = InstructionFormat::S_TYPE;
            instr.rd = 0;
            instr.rs1 = raw_rs1;
            instr.rs2 = raw_rs2;
            instr.funct3 = raw_funct3;
            instr.funct7 = 0;
            instr.immediate = extract_imm_s(instruction_word);
            instr.reg_write = false;
            instr.mem_read = false;
            instr.mem_write = true;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;

            if (raw_funct3 == funct3::WORD) {
                instr.id = InstructionID::SW;
                instr.alu_op = ALUOp::ADD;
            } else {
                instr.id = InstructionID::UNKNOWN;
                std::stringstream ss;
                ss << "Unsupported STORE funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                   << "): 0x" << std::setw(8) << std::setfill('0') << instruction_word;
                throw DecodeError(ss.str());
            }
            break;
        }

        case OpcodeType::BRANCH: {
            instr.format = InstructionFormat::B_TYPE;
            instr.rd = 0;
            instr.rs1 = raw_rs1;
            instr.rs2 = raw_rs2;
            instr.funct3 = raw_funct3;
            instr.funct7 = 0;
            instr.immediate = extract_imm_b(instruction_word);
            instr.reg_write = false;
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = true;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;
            instr.alu_op = ALUOp::SUB;

            if (raw_funct3 == funct3::BEQ) {
                instr.id = InstructionID::BEQ;
            } else if (raw_funct3 == funct3::BNE) {
                instr.id = InstructionID::BNE;
            } else {
                instr.id = InstructionID::UNKNOWN;
                std::stringstream ss;
                ss << "Unsupported BRANCH funct3 (0x" << std::hex << static_cast<int>(raw_funct3)
                   << "): 0x" << std::setw(8) << std::setfill('0') << instruction_word;
                throw DecodeError(ss.str());
            }
            break;
        }

        case OpcodeType::JAL: {
            instr.format = InstructionFormat::J_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = 0;
            instr.rs2 = 0;
            instr.funct3 = 0;
            instr.funct7 = 0;
            instr.immediate = extract_imm_j(instruction_word);
            instr.id = InstructionID::JAL;
            instr.alu_op = ALUOp::ADD;
            instr.reg_write = true;
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = true;
            instr.mem_to_reg = false;
            instr.is_halt = false;
            break;
        }

        case OpcodeType::LUI: {
            instr.format = InstructionFormat::U_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = 0;
            instr.rs2 = 0;
            instr.funct3 = 0;
            instr.funct7 = 0;
            instr.immediate = extract_imm_u(instruction_word);
            instr.id = InstructionID::LUI;
            instr.alu_op = ALUOp::PASS_B;
            instr.reg_write = true;
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;
            break;
        }

        case OpcodeType::AUIPC: {
            instr.format = InstructionFormat::U_TYPE;
            instr.rd = raw_rd;
            instr.rs1 = 0;
            instr.rs2 = 0;
            instr.funct3 = 0;
            instr.funct7 = 0;
            instr.immediate = extract_imm_u(instruction_word);
            instr.id = InstructionID::AUIPC;
            instr.alu_op = ALUOp::ADD_PC;
            instr.reg_write = true;
            instr.mem_read = false;
            instr.mem_write = false;
            instr.is_branch = false;
            instr.is_jump = false;
            instr.mem_to_reg = false;
            instr.is_halt = false;
            break;
        }

        case OpcodeType::SYSTEM: {
            if (raw_funct3 == 0) {
                instr.format = InstructionFormat::I_TYPE;
                instr.rd = 0;
                instr.rs1 = 0;
                instr.rs2 = 0;
                instr.funct3 = 0;
                instr.funct7 = 0;
                instr.immediate = 0;
                instr.id = InstructionID::HALT;
                instr.alu_op = ALUOp::NOP;
                instr.reg_write = false;
                instr.mem_read = false;
                instr.mem_write = false;
                instr.is_branch = false;
                instr.is_jump = false;
                instr.mem_to_reg = false;
                instr.is_halt = true;
            } else {
                instr.id = InstructionID::UNKNOWN;
                std::stringstream ss;
                ss << "Unsupported SYSTEM instruction (funct3 = 0x" << std::hex
                   << static_cast<int>(raw_funct3) << "): 0x"
                   << std::setw(8) << std::setfill('0') << instruction_word;
                throw DecodeError(ss.str());
            }
            break;
        }

        default: {
            instr.id = InstructionID::UNKNOWN;
            std::stringstream ss;
            ss << "Unknown opcode (0x" << std::hex << static_cast<int>(opcode)
               << ") in instruction: 0x" << std::setw(8) << std::setfill('0') << instruction_word;
            throw DecodeError(ss.str());
        }
    }

    return instr;
}

// ============================================================================
// Instruction Disassembler
// ============================================================================

std::string disassemble(const DecodedInstruction& instr) {
    switch (instr.id) {
        case InstructionID::NOP:
            return "NOP";

        case InstructionID::HALT:
            return "HALT";

        case InstructionID::ADD:
        case InstructionID::SUB:
        case InstructionID::AND:
        case InstructionID::OR:
        case InstructionID::XOR:
        case InstructionID::SLT:
            return instruction_name(instr.id) + " x" + std::to_string(static_cast<int>(instr.rd)) +
                   ", x" + std::to_string(static_cast<int>(instr.rs1)) +
                   ", x" + std::to_string(static_cast<int>(instr.rs2));

        case InstructionID::ADDI:
        case InstructionID::ANDI:
        case InstructionID::SLTI:
            return instruction_name(instr.id) + " x" + std::to_string(static_cast<int>(instr.rd)) +
                   ", x" + std::to_string(static_cast<int>(instr.rs1)) +
                   ", " + std::to_string(instr.immediate);

        case InstructionID::LW:
            return "LW x" + std::to_string(static_cast<int>(instr.rd)) + ", " +
                   std::to_string(instr.immediate) + "(x" +
                   std::to_string(static_cast<int>(instr.rs1)) + ")";

        case InstructionID::SW:
            return "SW x" + std::to_string(static_cast<int>(instr.rs2)) + ", " +
                   std::to_string(instr.immediate) + "(x" +
                   std::to_string(static_cast<int>(instr.rs1)) + ")";

        case InstructionID::BEQ:
        case InstructionID::BNE:
            return instruction_name(instr.id) + " x" + std::to_string(static_cast<int>(instr.rs1)) +
                   ", x" + std::to_string(static_cast<int>(instr.rs2)) +
                   ", " + std::to_string(instr.immediate);

        case InstructionID::JAL:
            return "JAL x" + std::to_string(static_cast<int>(instr.rd)) + ", " +
                   std::to_string(instr.immediate);

        case InstructionID::LUI:
        case InstructionID::AUIPC:
            return instruction_name(instr.id) + " x" + std::to_string(static_cast<int>(instr.rd)) +
                   ", " + std::to_string(instr.immediate);

        case InstructionID::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

// ============================================================================
// Instruction Encoders
// ============================================================================

Word encode_r_type(uint8_t funct7, Reg rs2, Reg rs1, uint8_t funct3, Reg rd, uint8_t opcode) {
    return ((static_cast<Word>(funct7) & 0x7FU) << 25) |
           ((static_cast<Word>(rs2)    & 0x1FU) << 20) |
           ((static_cast<Word>(rs1)    & 0x1FU) << 15) |
           ((static_cast<Word>(funct3) & 0x07U) << 12) |
           ((static_cast<Word>(rd)     & 0x1FU) <<  7) |
           (static_cast<Word>(opcode)  & 0x7FU);
}

Word encode_i_type(SWord imm, Reg rs1, uint8_t funct3, Reg rd, uint8_t opcode) {
    return ((static_cast<Word>(imm)    & 0xFFFU) << 20) |
           ((static_cast<Word>(rs1)    & 0x1FU)  << 15) |
           ((static_cast<Word>(funct3) & 0x07U)  << 12) |
           ((static_cast<Word>(rd)     & 0x1FU)  <<  7) |
           (static_cast<Word>(opcode)  & 0x7FU);
}

Word encode_s_type(SWord imm, Reg rs2, Reg rs1, uint8_t funct3, uint8_t opcode) {
    const Word uimm = static_cast<Word>(imm);
    return (((uimm >> 5) & 0x7FU)                << 25) |
           ((static_cast<Word>(rs2)    & 0x1FU)  << 20) |
           ((static_cast<Word>(rs1)    & 0x1FU)  << 15) |
           ((static_cast<Word>(funct3) & 0x07U)  << 12) |
           ((uimm & 0x1FU)                       <<  7) |
           (static_cast<Word>(opcode)  & 0x7FU);
}

Word encode_b_type(SWord imm, Reg rs2, Reg rs1, uint8_t funct3, uint8_t opcode) {
    const Word uimm = static_cast<Word>(imm);
    return (((uimm >> 12) & 0x01U)               << 31) |
           (((uimm >> 5)  & 0x3FU)               << 25) |
           ((static_cast<Word>(rs2)    & 0x1FU)  << 20) |
           ((static_cast<Word>(rs1)    & 0x1FU)  << 15) |
           ((static_cast<Word>(funct3) & 0x07U)  << 12) |
           (((uimm >> 1)  & 0x0FU)               <<  8) |
           (((uimm >> 11) & 0x01U)               <<  7) |
           (static_cast<Word>(opcode)  & 0x7FU);
}

Word encode_u_type(SWord imm, Reg rd, uint8_t opcode) {
    Word uimm = static_cast<Word>(imm);
    // If lower 12 bits are non-zero and upper 12 bits are zero, it is an unshifted 20-bit immediate
    if ((uimm & 0xFFFU) != 0 && (uimm & 0xFFF00000U) == 0) {
        uimm <<= 12;
    }
    return (uimm & 0xFFFFF000U) |
           ((static_cast<Word>(rd)    & 0x1FU) << 7) |
           (static_cast<Word>(opcode) & 0x7FU);
}

Word encode_j_type(SWord imm, Reg rd, uint8_t opcode) {
    const Word uimm = static_cast<Word>(imm);
    return (((uimm >> 20) & 0x01U)              << 31) |
           (((uimm >> 1)  & 0x3FFU)             << 21) |
           (((uimm >> 11) & 0x01U)              << 20) |
           (((uimm >> 12) & 0xFFU)              << 12) |
           ((static_cast<Word>(rd)    & 0x1FU)  <<  7) |
           (static_cast<Word>(opcode) & 0x7FU);
}

} // namespace microarchsim

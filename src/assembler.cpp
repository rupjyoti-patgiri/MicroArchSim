/// @file assembler.cpp
/// @brief Two-pass assembler for the RV32I-Edu instruction subset.

#include "assembler.h"
#include "instruction.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace microarchsim {

// ============================================================================
// Helper: Trim whitespace
// ============================================================================

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// ============================================================================
// ABI register name mapping
// ============================================================================

static const std::unordered_map<std::string, Reg> abi_names = {
    {"ZERO", 0}, {"RA", 1}, {"SP", 2}, {"GP", 3}, {"TP", 4},
    {"T0", 5}, {"T1", 6}, {"T2", 7},
    {"S0", 8}, {"FP", 8}, {"S1", 9},
    {"A0", 10}, {"A1", 11}, {"A2", 12}, {"A3", 13},
    {"A4", 14}, {"A5", 15}, {"A6", 16}, {"A7", 17},
    {"S2", 18}, {"S3", 19}, {"S4", 20}, {"S5", 21},
    {"S6", 22}, {"S7", 23}, {"S8", 24}, {"S9", 25},
    {"S10", 26}, {"S11", 27},
    {"T3", 28}, {"T4", 29}, {"T5", 30}, {"T6", 31}
};

// ============================================================================
// Parse source into lines
// ============================================================================

std::vector<Assembler::ParsedLine> Assembler::parse_lines(const std::string& source) const {
    std::vector<ParsedLine> lines;
    std::istringstream stream(source);
    std::string raw_line;
    int line_num = 0;

    while (std::getline(stream, raw_line)) {
        line_num++;

        // Strip comments (# or ;)
        auto comment_pos = raw_line.find('#');
        if (comment_pos != std::string::npos)
            raw_line = raw_line.substr(0, comment_pos);
        comment_pos = raw_line.find(';');
        if (comment_pos != std::string::npos)
            raw_line = raw_line.substr(0, comment_pos);

        std::string trimmed = trim(raw_line);
        if (trimmed.empty()) continue;

        ParsedLine pl;
        pl.line_number = line_num;
        pl.is_directive = false;

        // Check for label (ends with ':')
        auto colon_pos = trimmed.find(':');
        if (colon_pos != std::string::npos) {
            pl.label = trim(trimmed.substr(0, colon_pos));
            trimmed = trim(trimmed.substr(colon_pos + 1));
            if (trimmed.empty()) {
                // Label-only line
                lines.push_back(pl);
                continue;
            }
        }

        // Check for directive
        if (!trimmed.empty() && trimmed[0] == '.') {
            pl.is_directive = true;
            auto space_pos = trimmed.find_first_of(" \t");
            if (space_pos != std::string::npos) {
                pl.directive = to_upper(trim(trimmed.substr(0, space_pos)));
                pl.directive_arg = trim(trimmed.substr(space_pos + 1));
            } else {
                pl.directive = to_upper(trimmed);
            }
            lines.push_back(pl);
            continue;
        }

        // Parse instruction: split by spaces/commas
        // Replace commas with spaces for easier tokenization
        std::replace(trimmed.begin(), trimmed.end(), ',', ' ');
        std::istringstream token_stream(trimmed);
        std::string token;

        // First token is the opcode
        token_stream >> token;
        pl.opcode = to_upper(token);

        // Remaining tokens are operands
        while (token_stream >> token) {
            pl.operands.push_back(trim(token));
        }

        lines.push_back(pl);
    }

    return lines;
}

// ============================================================================
// First pass: collect labels
// ============================================================================

std::unordered_map<std::string, Addr> Assembler::collect_labels(
    const std::vector<ParsedLine>& lines) const {

    std::unordered_map<std::string, Addr> labels;
    Addr current_addr = INSTRUCTION_BASE;
    bool in_data_section = false;
    Addr data_addr = DATA_BASE;

    for (const auto& line : lines) {
        if (line.is_directive) {
            if (line.directive == ".DATA" || line.directive == ".SECTION" ) {
                in_data_section = true;
                continue;
            }
            if (line.directive == ".TEXT") {
                in_data_section = false;
                continue;
            }
            if (line.directive == ".WORD") {
                if (!line.label.empty()) {
                    labels[line.label] = data_addr;
                }
                data_addr += 4;
                continue;
            }
        }

        if (!line.label.empty() && !in_data_section) {
            if (labels.count(line.label)) {
                throw AssemblyError("Duplicate label '" + line.label + "' at line " +
                                    std::to_string(line.line_number));
            }
            labels[line.label] = current_addr;
        }

        if (!line.opcode.empty() && !in_data_section) {
            current_addr += 4;  // Each instruction is 4 bytes
        }
    }

    return labels;
}

// ============================================================================
// Register parsing
// ============================================================================

Reg Assembler::parse_register(const std::string& s, int line_number) const {
    std::string upper = to_upper(trim(s));

    // Try xN format
    if (upper.size() >= 2 && upper[0] == 'X') {
        try {
            int reg_num = std::stoi(upper.substr(1));
            if (reg_num < 0 || reg_num >= NUM_REGISTERS) {
                throw AssemblyError("Register number out of range at line " +
                                    std::to_string(line_number) + ": " + s);
            }
            return static_cast<Reg>(reg_num);
        } catch (const std::invalid_argument&) {
            // Fall through to ABI names
        }
    }

    // Try ABI names
    auto it = abi_names.find(upper);
    if (it != abi_names.end()) {
        return it->second;
    }

    throw AssemblyError("Invalid register '" + s + "' at line " +
                        std::to_string(line_number));
}

// ============================================================================
// Immediate parsing
// ============================================================================

SWord Assembler::parse_immediate(const std::string& s,
                                  const std::unordered_map<std::string, Addr>& labels,
                                  Addr current_pc, int line_number) const {
    std::string trimmed = trim(s);

    // Check for label reference
    auto it = labels.find(trimmed);
    if (it != labels.end()) {
        // Return PC-relative offset for branches/jumps
        return static_cast<SWord>(it->second) - static_cast<SWord>(current_pc);
    }

    // Try hex
    if (trimmed.size() > 2 && trimmed[0] == '0' &&
        (trimmed[1] == 'x' || trimmed[1] == 'X')) {
        try {
            return static_cast<SWord>(std::stoul(trimmed, nullptr, 16));
        } catch (...) {
            throw AssemblyError("Invalid hex immediate '" + s + "' at line " +
                                std::to_string(line_number));
        }
    }

    // Try decimal (possibly negative)
    try {
        return static_cast<SWord>(std::stoi(trimmed));
    } catch (...) {
        throw AssemblyError("Invalid immediate '" + s + "' at line " +
                            std::to_string(line_number));
    }
}

// ============================================================================
// Memory operand parsing: offset(reg)
// ============================================================================

void Assembler::parse_memory_operand(const std::string& s, SWord& offset, Reg& reg,
                                      const std::unordered_map<std::string, Addr>& labels,
                                      Addr current_pc, int line_number) const {
    auto paren_open = s.find('(');
    auto paren_close = s.find(')');

    if (paren_open == std::string::npos || paren_close == std::string::npos ||
        paren_close <= paren_open) {
        throw AssemblyError("Invalid memory operand '" + s + "' at line " +
                            std::to_string(line_number) + ". Expected format: offset(reg)");
    }

    std::string offset_str = trim(s.substr(0, paren_open));
    std::string reg_str = trim(s.substr(paren_open + 1, paren_close - paren_open - 1));

    if (offset_str.empty()) {
        offset = 0;
    } else {
        offset = parse_immediate(offset_str, labels, current_pc, line_number);
    }

    reg = parse_register(reg_str, line_number);
}

// ============================================================================
// Encode a single instruction
// ============================================================================

Word Assembler::encode_instruction(const ParsedLine& line,
                                    const std::unordered_map<std::string, Addr>& labels,
                                    Addr current_pc) const {
    const auto& op = line.opcode;
    const auto& ops = line.operands;
    int ln = line.line_number;

    auto expect_operands = [&](size_t n) {
        if (ops.size() != n) {
            throw AssemblyError(op + " expects " + std::to_string(n) + " operands at line " +
                                std::to_string(ln) + ", got " + std::to_string(ops.size()));
        }
    };

    // R-type: OP rd, rs1, rs2
    if (op == "ADD" || op == "SUB" || op == "AND" || op == "OR" ||
        op == "XOR" || op == "SLT") {
        expect_operands(3);
        Reg rd = parse_register(ops[0], ln);
        Reg rs1 = parse_register(ops[1], ln);
        Reg rs2 = parse_register(ops[2], ln);

        uint8_t f7 = funct7::NORMAL;
        uint8_t f3 = 0;

        if (op == "ADD")      { f3 = funct3::ADD_SUB; f7 = funct7::NORMAL; }
        else if (op == "SUB") { f3 = funct3::ADD_SUB; f7 = funct7::ALT; }
        else if (op == "AND") { f3 = funct3::AND; }
        else if (op == "OR")  { f3 = funct3::OR; }
        else if (op == "XOR") { f3 = funct3::XOR; }
        else if (op == "SLT") { f3 = funct3::SLT; }

        return encode_r_type(f7, rs2, rs1, f3, rd, static_cast<uint8_t>(OpcodeType::R_TYPE));
    }

    // I-type: OP rd, rs1, imm
    if (op == "ADDI" || op == "ANDI" || op == "SLTI") {
        expect_operands(3);
        Reg rd = parse_register(ops[0], ln);
        Reg rs1 = parse_register(ops[1], ln);
        SWord imm = parse_immediate(ops[2], labels, current_pc, ln);

        uint8_t f3 = 0;
        if (op == "ADDI")      f3 = funct3::ADD_SUB;
        else if (op == "ANDI") f3 = funct3::AND;
        else if (op == "SLTI") f3 = funct3::SLT;

        return encode_i_type(imm, rs1, f3, rd, static_cast<uint8_t>(OpcodeType::I_TYPE));
    }

    // NOP
    if (op == "NOP") {
        expect_operands(0);
        return NOP_INSTRUCTION;
    }

    // HALT
    if (op == "HALT") {
        expect_operands(0);
        return HALT_INSTRUCTION;
    }

    // LW: LW rd, offset(rs1)
    if (op == "LW") {
        expect_operands(2);
        Reg rd = parse_register(ops[0], ln);
        SWord offset;
        Reg rs1;
        parse_memory_operand(ops[1], offset, rs1, labels, current_pc, ln);
        return encode_i_type(offset, rs1, funct3::WORD, rd,
                             static_cast<uint8_t>(OpcodeType::LOAD));
    }

    // SW: SW rs2, offset(rs1)
    if (op == "SW") {
        expect_operands(2);
        Reg rs2 = parse_register(ops[0], ln);
        SWord offset;
        Reg rs1;
        parse_memory_operand(ops[1], offset, rs1, labels, current_pc, ln);
        return encode_s_type(offset, rs2, rs1, funct3::WORD,
                             static_cast<uint8_t>(OpcodeType::STORE));
    }

    // BEQ, BNE: OP rs1, rs2, label/offset
    if (op == "BEQ" || op == "BNE") {
        expect_operands(3);
        Reg rs1 = parse_register(ops[0], ln);
        Reg rs2 = parse_register(ops[1], ln);
        SWord imm = parse_immediate(ops[2], labels, current_pc, ln);

        uint8_t f3 = (op == "BEQ") ? funct3::BEQ : funct3::BNE;
        return encode_b_type(imm, rs2, rs1, f3,
                             static_cast<uint8_t>(OpcodeType::BRANCH));
    }

    // JAL: JAL rd, label/offset
    if (op == "JAL") {
        if (ops.size() == 1) {
            // JAL label → JAL x1, label
            SWord imm = parse_immediate(ops[0], labels, current_pc, ln);
            return encode_j_type(imm, 1, static_cast<uint8_t>(OpcodeType::JAL));
        }
        expect_operands(2);
        Reg rd = parse_register(ops[0], ln);
        SWord imm = parse_immediate(ops[1], labels, current_pc, ln);
        return encode_j_type(imm, rd, static_cast<uint8_t>(OpcodeType::JAL));
    }

    // LUI: LUI rd, imm
    if (op == "LUI") {
        expect_operands(2);
        Reg rd = parse_register(ops[0], ln);
        SWord imm = parse_immediate(ops[1], labels, current_pc, ln);
        // LUI immediate is the upper 20 bits; the assembler accepts the raw upper value
        return encode_u_type(imm << 12, rd, static_cast<uint8_t>(OpcodeType::LUI));
    }

    // AUIPC: AUIPC rd, imm
    if (op == "AUIPC") {
        expect_operands(2);
        Reg rd = parse_register(ops[0], ln);
        SWord imm = parse_immediate(ops[1], labels, current_pc, ln);
        return encode_u_type(imm << 12, rd, static_cast<uint8_t>(OpcodeType::AUIPC));
    }

    // J pseudo-instruction: J label → JAL x0, label
    if (op == "J") {
        expect_operands(1);
        SWord imm = parse_immediate(ops[0], labels, current_pc, ln);
        return encode_j_type(imm, 0, static_cast<uint8_t>(OpcodeType::JAL));
    }

    throw AssemblyError("Unknown instruction '" + op + "' at line " +
                        std::to_string(ln));
}

// ============================================================================
// Second pass: encode all instructions
// ============================================================================

std::vector<Word> Assembler::encode_instructions(
    const std::vector<ParsedLine>& lines,
    const std::unordered_map<std::string, Addr>& labels) const {

    std::vector<Word> instructions;
    Addr current_pc = INSTRUCTION_BASE;
    bool in_data_section = false;

    for (const auto& line : lines) {
        if (line.is_directive) {
            if (line.directive == ".DATA" || line.directive == ".SECTION") {
                in_data_section = true;
            } else if (line.directive == ".TEXT") {
                in_data_section = false;
            }
            continue;
        }

        if (in_data_section) continue;
        if (line.opcode.empty()) continue;  // Label-only line

        Word encoded = encode_instruction(line, labels, current_pc);
        instructions.push_back(encoded);
        current_pc += 4;
    }

    return instructions;
}

// ============================================================================
// Collect data section
// ============================================================================

std::vector<Word> Assembler::collect_data(const std::vector<ParsedLine>& lines) const {
    std::vector<Word> data;
    bool in_data_section = false;

    for (const auto& line : lines) {
        if (line.is_directive) {
            if (line.directive == ".DATA") {
                in_data_section = true;
                continue;
            }
            if (line.directive == ".TEXT") {
                in_data_section = false;
                continue;
            }
            if (line.directive == ".WORD" && in_data_section) {
                try {
                    // Support hex and decimal
                    std::string arg = trim(line.directive_arg);
                    Word value;
                    if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
                        value = static_cast<Word>(std::stoul(arg, nullptr, 16));
                    } else {
                        value = static_cast<Word>(std::stoi(arg));
                    }
                    data.push_back(value);
                } catch (...) {
                    throw AssemblyError(".word directive has invalid value: " + line.directive_arg +
                                        " at line " + std::to_string(line.line_number));
                }
            }
        }
    }

    return data;
}

// ============================================================================
// Main assemble entry points
// ============================================================================

AssemblyResult Assembler::assemble(const std::string& source) const {
    auto lines = parse_lines(source);
    auto labels = collect_labels(lines);
    auto instructions = encode_instructions(lines, labels);
    auto data = collect_data(lines);

    return {instructions, data, labels};
}

AssemblyResult Assembler::assemble_file(const std::string& filepath) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw AssemblyError("Cannot open file: " + filepath);
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    return assemble(source);
}

} // namespace microarchsim

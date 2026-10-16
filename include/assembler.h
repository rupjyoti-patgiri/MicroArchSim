#pragma once

/// @file assembler.h
/// @brief Two-pass assembler for the RV32I-Edu instruction subset.

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace microarchsim {

/// Result of assembling a program.
struct AssemblyResult {
    std::vector<Word> instructions;   ///< Encoded instruction words
    std::vector<Word> data;           ///< Data section words
    std::unordered_map<std::string, Addr> labels;  ///< Label → address map
};

/// Two-pass assembler for the RV32I-Edu subset.
class Assembler {
public:
    /// Assemble source text into instruction and data words.
    /// @throws AssemblyError on syntax or encoding errors.
    AssemblyResult assemble(const std::string& source) const;

    /// Assemble from a file.
    AssemblyResult assemble_file(const std::string& filepath) const;

private:
    /// Represents a single parsed line.
    struct ParsedLine {
        int line_number;
        std::string label;         ///< Label defined on this line (empty if none)
        std::string opcode;        ///< Instruction mnemonic (uppercase)
        std::vector<std::string> operands;  ///< Operand strings
        bool is_directive;         ///< Is this a directive (.data, .word, etc.)?
        std::string directive;     ///< Directive name
        std::string directive_arg; ///< Directive argument
    };

    /// Parse source text into lines.
    std::vector<ParsedLine> parse_lines(const std::string& source) const;

    /// First pass: collect all labels and their addresses.
    std::unordered_map<std::string, Addr> collect_labels(
        const std::vector<ParsedLine>& lines) const;

    /// Second pass: encode all instructions.
    std::vector<Word> encode_instructions(
        const std::vector<ParsedLine>& lines,
        const std::unordered_map<std::string, Addr>& labels) const;

    /// Collect data section words.
    std::vector<Word> collect_data(const std::vector<ParsedLine>& lines) const;

    /// Parse a register operand (e.g., "x5" or "t0").
    Reg parse_register(const std::string& s, int line_number) const;

    /// Parse an immediate or label reference.
    SWord parse_immediate(const std::string& s,
                          const std::unordered_map<std::string, Addr>& labels,
                          Addr current_pc, int line_number) const;

    /// Parse memory operand "offset(reg)" into offset and register.
    void parse_memory_operand(const std::string& s, SWord& offset, Reg& reg,
                              const std::unordered_map<std::string, Addr>& labels,
                              Addr current_pc, int line_number) const;

    /// Encode a single instruction.
    Word encode_instruction(const ParsedLine& line,
                            const std::unordered_map<std::string, Addr>& labels,
                            Addr current_pc) const;
};

} // namespace microarchsim

#pragma once

/// @file memory.h
/// @brief Byte-addressable memory subsystem for MicroArchSim.

#include "types.h"
#include <vector>
#include <ostream>

namespace microarchsim {

/// Byte-addressable memory subsystem.
/// Supports word-aligned reads/writes and byte-level access.
class Memory {
public:
    /// Construct memory with given size in bytes.
    explicit Memory(uint32_t size = 1 << 20);

    /// Read a 32-bit word from a word-aligned address.
    /// Throws MemoryError on misalignment or out-of-bounds.
    Word read_word(Addr addr) const;

    /// Write a 32-bit word to a word-aligned address.
    /// Throws MemoryError on misalignment or out-of-bounds.
    void write_word(Addr addr, Word value);

    /// Read a single byte.
    uint8_t read_byte(Addr addr) const;

    /// Write a single byte.
    void write_byte(Addr addr, uint8_t value);

    /// Load a sequence of instruction words starting at base address.
    void load_program(const std::vector<Word>& instructions, Addr base = INSTRUCTION_BASE);

    /// Load data words starting at base address.
    void load_data(const std::vector<Word>& data, Addr base = DATA_BASE);

    /// Reset all memory to zero.
    void reset();

    /// Get memory size.
    uint32_t size() const;

    /// Dump non-zero memory contents to stream (for data section).
    void dump(std::ostream& os, Addr start, Addr end) const;

    /// Snapshot of a memory range for comparison.
    std::vector<uint8_t> snapshot(Addr start, uint32_t length) const;

private:
    std::vector<uint8_t> data_;

    /// Check address is within bounds.
    void check_bounds(Addr addr, uint32_t access_size) const;

    /// Check word alignment.
    void check_alignment(Addr addr) const;
};

} // namespace microarchsim

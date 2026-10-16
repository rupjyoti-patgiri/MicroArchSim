#pragma once

/// @file register_file.h
/// @brief 32-entry RISC-V register file with x0 hardwired to zero.

#include "types.h"
#include <array>
#include <ostream>

namespace microarchsim {

/// 32-entry register file with x0 hardwired to zero.
class RegisterFile {
public:
    RegisterFile();

    /// Read register value. x0 always returns 0.
    Word read(Reg reg) const;

    /// Write register value. Writes to x0 are silently ignored.
    void write(Reg reg, Word value);

    /// Reset all registers to zero.
    void reset();

    /// Print all register values to stream.
    void dump(std::ostream& os) const;

    /// Snapshot of all register values for comparison.
    std::array<Word, NUM_REGISTERS> snapshot() const;

    /// Compare two register files for equality.
    bool operator==(const RegisterFile& other) const;
    bool operator!=(const RegisterFile& other) const;

private:
    std::array<Word, NUM_REGISTERS> regs_;
};

} // namespace microarchsim

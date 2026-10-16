/// @file register_file.cpp
/// @brief 32-entry RISC-V register file implementation.

#include "register_file.h"
#include <iomanip>
#include <sstream>
#include <string>

namespace microarchsim {

RegisterFile::RegisterFile() {
    reset();
}

Word RegisterFile::read(Reg reg) const {
    if (reg >= NUM_REGISTERS) {
        throw SimulatorError("Register index out of bounds: " + std::to_string(reg));
    }
    if (reg == 0) {
        return 0;
    }
    return regs_[reg];
}

void RegisterFile::write(Reg reg, Word value) {
    if (reg >= NUM_REGISTERS) {
        throw SimulatorError("Register index out of bounds: " + std::to_string(reg));
    }
    if (reg == 0) {
        return;
    }
    regs_[reg] = value;
}

void RegisterFile::reset() {
    regs_.fill(0);
}

void RegisterFile::dump(std::ostream& os) const {
    auto orig_flags = os.flags();
    auto orig_fill = os.fill();

    for (int i = 0; i < NUM_REGISTERS; ++i) {
        std::string reg_name = "x" + std::to_string(i);
        os << std::left << std::setw(4) << std::setfill(' ') << reg_name;
        os << "= 0x" << std::right << std::setw(8) << std::setfill('0')
           << std::hex << std::uppercase << regs_[static_cast<size_t>(i)];

        if ((i + 1) % 4 == 0) {
            os << '\n';
        } else {
            os << "   ";
        }
    }

    os.flags(orig_flags);
    os.fill(orig_fill);
}

std::array<Word, NUM_REGISTERS> RegisterFile::snapshot() const {
    return regs_;
}

bool RegisterFile::operator==(const RegisterFile& other) const {
    return regs_ == other.regs_;
}

bool RegisterFile::operator!=(const RegisterFile& other) const {
    return !(*this == other);
}

} // namespace microarchsim

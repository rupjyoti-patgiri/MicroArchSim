/// @file memory.cpp
/// @brief Byte-addressable memory subsystem implementation.

#include "memory.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace microarchsim {

Memory::Memory(uint32_t size)
    : data_(size, 0)
{
}

Word Memory::read_word(Addr addr) const {
    check_alignment(addr);
    check_bounds(addr, 4);

    return static_cast<Word>(data_[addr])
         | (static_cast<Word>(data_[addr + 1]) << 8)
         | (static_cast<Word>(data_[addr + 2]) << 16)
         | (static_cast<Word>(data_[addr + 3]) << 24);
}

void Memory::write_word(Addr addr, Word value) {
    check_alignment(addr);
    check_bounds(addr, 4);

    data_[addr]     = static_cast<uint8_t>(value & 0xFF);
    data_[addr + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data_[addr + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data_[addr + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint8_t Memory::read_byte(Addr addr) const {
    check_bounds(addr, 1);
    return data_[addr];
}

void Memory::write_byte(Addr addr, uint8_t value) {
    check_bounds(addr, 1);
    data_[addr] = value;
}

void Memory::load_program(const std::vector<Word>& instructions, Addr base) {
    for (size_t i = 0; i < instructions.size(); ++i) {
        write_word(base + static_cast<Addr>(i * 4), instructions[i]);
    }
}

void Memory::load_data(const std::vector<Word>& data, Addr base) {
    for (size_t i = 0; i < data.size(); ++i) {
        write_word(base + static_cast<Addr>(i * 4), data[i]);
    }
}

void Memory::reset() {
    std::fill(data_.begin(), data_.end(), uint8_t{0});
}

uint32_t Memory::size() const {
    return static_cast<uint32_t>(data_.size());
}

void Memory::dump(std::ostream& os, Addr start, Addr end) const {
    auto orig_flags = os.flags();
    auto orig_fill = os.fill();

    for (uint64_t addr = start; addr < end; addr += 4) {
        Word val = read_word(static_cast<Addr>(addr));
        if (val != 0) {
            os << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << static_cast<Addr>(addr)
               << ": 0x" << std::setw(8) << val << '\n';
        }
    }

    os.flags(orig_flags);
    os.fill(orig_fill);
}

std::vector<uint8_t> Memory::snapshot(Addr start, uint32_t length) const {
    if (length == 0) {
        check_bounds(start, 0);
        return {};
    }
    check_bounds(start, length);
    return std::vector<uint8_t>(data_.begin() + start, data_.begin() + start + length);
}

void Memory::check_bounds(Addr addr, uint32_t access_size) const {
    if (static_cast<uint64_t>(addr) + access_size > data_.size()) {
        std::ostringstream ss;
        ss << "Memory access out of bounds at address 0x"
           << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << addr;
        throw MemoryError(ss.str());
    }
}

void Memory::check_alignment(Addr addr) const {
    if (addr % 4 != 0) {
        std::ostringstream ss;
        ss << "Memory access misaligned at address 0x"
           << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << addr;
        throw MemoryError(ss.str());
    }
}

} // namespace microarchsim

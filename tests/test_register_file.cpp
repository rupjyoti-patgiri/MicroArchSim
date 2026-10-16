#include <gtest/gtest.h>
#include "register_file.h"
#include "types.h"

using namespace microarchsim;

TEST(RegisterFileTest, ZeroRegisterImmutable) {
    RegisterFile rf;
    EXPECT_EQ(rf.read(0), 0u);

    // Attempt to write non-zero value to x0
    rf.write(0, 0xDEADBEEF);
    EXPECT_EQ(rf.read(0), 0u);

    rf.write(0, 0xFFFFFFFF);
    EXPECT_EQ(rf.read(0), 0u);
}

TEST(RegisterFileTest, GeneralPurposeRegistersReadWrite) {
    RegisterFile rf;

    for (Reg r = 1; r < NUM_REGISTERS; ++r) {
        Word val = 0x1000u + r;
        rf.write(r, val);
        EXPECT_EQ(rf.read(r), val);
    }
}

TEST(RegisterFileTest, ResetClearsAllRegisters) {
    RegisterFile rf;
    rf.write(1, 0x12345678);
    rf.write(31, 0x87654321);

    rf.reset();
    for (Reg r = 0; r < NUM_REGISTERS; ++r) {
        EXPECT_EQ(rf.read(r), 0u);
    }
}

TEST(RegisterFileTest, OutOfBoundsThrows) {
    RegisterFile rf;
    EXPECT_THROW(rf.read(32), SimulatorError);
    EXPECT_THROW(rf.write(32, 10), SimulatorError);
}

TEST(RegisterFileTest, EqualityAndSnapshot) {
    RegisterFile rf1, rf2;
    rf1.write(5, 42);
    rf2.write(5, 42);
    EXPECT_EQ(rf1, rf2);

    rf2.write(5, 43);
    EXPECT_NE(rf1, rf2);

    auto snap = rf1.snapshot();
    EXPECT_EQ(snap[5], 42u);
    EXPECT_EQ(snap[0], 0u);
}

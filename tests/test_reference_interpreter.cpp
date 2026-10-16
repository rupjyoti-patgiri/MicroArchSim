#include <gtest/gtest.h>
#include "reference_interpreter.h"
#include "assembler.h"
#include "types.h"

using namespace microarchsim;

class ReferenceInterpreterTest : public ::testing::Test {
protected:
    ReferenceInterpreter sim;
    Assembler assem;
};

TEST_F(ReferenceInterpreterTest, BasicArithmeticAndImm) {
    std::string asm_code = R"(
        ADDI x1, x0, 15
        ADDI x2, x0, 25
        ADD  x3, x1, x2
        SUB  x4, x3, x1
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);
    sim.load_program(res.instructions);
    sim.run();

    EXPECT_EQ(sim.registers().read(1), 15u);
    EXPECT_EQ(sim.registers().read(2), 25u);
    EXPECT_EQ(sim.registers().read(3), 40u);
    EXPECT_EQ(sim.registers().read(4), 25u);
    EXPECT_EQ(sim.registers().read(0), 0u);
}

TEST_F(ReferenceInterpreterTest, BitwiseLogicalOperations) {
    std::string asm_code = R"(
        ADDI x1, x0, 15
        ADDI x2, x0, 51
        AND  x3, x1, x2
        OR   x4, x1, x2
        XOR  x5, x1, x2
        ANDI x6, x1, 7
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);
    sim.load_program(res.instructions);
    sim.run();

    EXPECT_EQ(sim.registers().read(3), 3u);
    EXPECT_EQ(sim.registers().read(4), 63u);
    EXPECT_EQ(sim.registers().read(5), 60u);
    EXPECT_EQ(sim.registers().read(6), 7u);
}

TEST_F(ReferenceInterpreterTest, MemoryLoadAndStore) {
    std::string asm_code = R"(
        ADDI x1, x0, 256
        ADDI x2, x0, 42
        SW   x2, 0(x1)
        LW   x3, 0(x1)
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);
    sim.load_program(res.instructions);
    sim.run();

    EXPECT_EQ(sim.registers().read(3), 42u);
    EXPECT_EQ(sim.memory().read_word(256), 42u);
}

TEST_F(ReferenceInterpreterTest, BranchLoopCountdown) {
    std::string asm_code = R"(
        ADDI x1, x0, 5
        ADDI x2, x0, 0
    loop:
        ADDI x2, x2, 10
        ADDI x1, x1, -1
        BNE  x1, x0, loop
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);
    sim.load_program(res.instructions);
    sim.run();

    EXPECT_EQ(sim.registers().read(1), 0u);
    EXPECT_EQ(sim.registers().read(2), 50u);
}

TEST_F(ReferenceInterpreterTest, JumpAndLink) {
    std::string asm_code = R"(
        ADDI x1, x0, 10
        JAL  x5, target
        ADDI x1, x0, 99
    target:
        ADDI x2, x0, 20
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);
    sim.load_program(res.instructions);
    sim.run();

    EXPECT_EQ(sim.registers().read(1), 10u);
    EXPECT_EQ(sim.registers().read(2), 20u);
    EXPECT_EQ(sim.registers().read(5), 8u);
}

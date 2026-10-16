#include <gtest/gtest.h>
#include "pipeline.h"
#include "reference_interpreter.h"
#include "assembler.h"
#include "types.h"

using namespace microarchsim;

class DifferentialTest : public ::testing::Test {
protected:
    Assembler assem;

    void compare_simulators(const std::string& asm_code) {
        AssemblyResult res = assem.assemble(asm_code);

        // Run reference interpreter
        ReferenceInterpreter ref;
        ref.load_program(res.instructions);
        ref.run();

        // Run pipeline with forwarding
        SimConfig pipe_config;
        pipe_config.forwarding_enabled = true;
        pipe_config.cache_enabled = false;
        Pipeline pipe(pipe_config);
        pipe.load_program(res.instructions);
        pipe.run();

        // Verify identical register state across all 32 registers
        for (Reg r = 0; r < NUM_REGISTERS; ++r) {
            EXPECT_EQ(pipe.registers().read(r), ref.registers().read(r))
                << "Mismatch at register x" << static_cast<int>(r);
        }
    }
};

TEST_F(DifferentialTest, ArithmeticAndImmediates) {
    std::string asm_code = R"(
        ADDI x1, x0, 100
        ADDI x2, x0, 200
        ADD  x3, x1, x2
        SUB  x4, x3, x1
        ANDI x5, x4, 85
        HALT
    )";
    compare_simulators(asm_code);
}

TEST_F(DifferentialTest, MemorySpillAndReload) {
    std::string asm_code = R"(
        ADDI x1, x0, 512
        ADDI x2, x0, 1234
        ADDI x3, x0, 5678
        SW   x2, 0(x1)
        SW   x3, 4(x1)
        LW   x4, 0(x1)
        LW   x5, 4(x1)
        ADD  x6, x4, x5
        HALT
    )";
    compare_simulators(asm_code);
}

TEST_F(DifferentialTest, ComplexLoopWithBranching) {
    std::string asm_code = R"(
        ADDI x1, x0, 10
        ADDI x2, x0, 0
        ADDI x3, x0, 1
    loop:
        ADD  x2, x2, x1
        SUB  x1, x1, x3
        BNE  x1, x0, loop
        HALT
    )";
    compare_simulators(asm_code);
}

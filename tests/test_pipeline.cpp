#include <gtest/gtest.h>
#include "pipeline.h"
#include "assembler.h"
#include "types.h"

using namespace microarchsim;

class PipelineTest : public ::testing::Test {
protected:
    Assembler assem;
};

TEST_F(PipelineTest, IndependentInstructionsThroughput) {
    // 5 independent instructions + HALT
    std::string asm_code = R"(
        ADDI x1, x0, 10
        ADDI x2, x0, 20
        ADDI x3, x0, 30
        ADDI x4, x0, 40
        ADDI x5, x0, 50
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(1), 10u);
    EXPECT_EQ(pipe.registers().read(2), 20u);
    EXPECT_EQ(pipe.registers().read(3), 30u);
    EXPECT_EQ(pipe.registers().read(4), 40u);
    EXPECT_EQ(pipe.registers().read(5), 50u);
    EXPECT_EQ(pipe.registers().read(0), 0u);

    Statistics stats = pipe.get_stats();
    EXPECT_EQ(stats.instructions_retired, 6u);
    EXPECT_EQ(stats.total_cycles, 10u);
    EXPECT_EQ(stats.data_hazard_stalls, 0u);
}

TEST_F(PipelineTest, BranchFlushingBehavior) {
    std::string asm_code = R"(
        ADDI x1, x0, 1
        BEQ  x1, x1, taken
        ADDI x2, x0, 99
        ADDI x3, x0, 99
    taken:
        ADDI x4, x0, 42
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(1), 1u);
    // Flushed instructions must NOT have executed or modified architectural state
    EXPECT_EQ(pipe.registers().read(2), 0u);
    EXPECT_EQ(pipe.registers().read(3), 0u);
    EXPECT_EQ(pipe.registers().read(4), 42u);

    Statistics stats = pipe.get_stats();
    EXPECT_GT(stats.control_flush_cycles, 0u);
}

TEST_F(PipelineTest, ConsecutiveLoopExecution) {
    std::string asm_code = R"(
        ADDI x1, x0, 4
        ADDI x2, x0, 0
    loop:
        ADDI x2, x2, 5
        ADDI x1, x1, -1
        BNE  x1, x0, loop
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(1), 0u);
    EXPECT_EQ(pipe.registers().read(2), 20u);
}

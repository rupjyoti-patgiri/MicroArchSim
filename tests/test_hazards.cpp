#include <gtest/gtest.h>
#include "pipeline.h"
#include "assembler.h"
#include "types.h"

using namespace microarchsim;

class HazardsTest : public ::testing::Test {
protected:
    Assembler assem;
};

TEST_F(HazardsTest, ExMemForwarding) {
    // RAW hazard resolved by EX/MEM -> EX forwarding
    // ADD writes x1, next instruction reads x1 immediately
    std::string asm_code = R"(
        ADDI x1, x0, 10
        ADDI x2, x1, 5
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
    EXPECT_EQ(pipe.registers().read(2), 15u);
    EXPECT_EQ(pipe.get_stats().data_hazard_stalls, 0u);
}

TEST_F(HazardsTest, MemWbForwarding) {
    // RAW hazard with 1 independent instruction in between:
    // ADDI x1 writes x1
    // ADDI x3 is independent
    // ADDI x2 reads x1 (comes from MEM/WB stage)
    std::string asm_code = R"(
        ADDI x1, x0, 10
        ADDI x3, x0, 100
        ADDI x2, x1, 5
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
    EXPECT_EQ(pipe.registers().read(3), 100u);
    EXPECT_EQ(pipe.registers().read(2), 15u);
    EXPECT_EQ(pipe.get_stats().data_hazard_stalls, 0u);
}

TEST_F(HazardsTest, ForwardingPriorityConsecutiveWrites) {
    // Both EX/MEM and MEM/WB write to x1!
    // The youngest instruction (EX/MEM) MUST have priority.
    std::string asm_code = R"(
        ADDI x1, x0, 10
        ADDI x1, x0, 20
        ADDI x2, x1, 5
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(1), 20u);
    EXPECT_EQ(pipe.registers().read(2), 25u); // 20 + 5, NOT 10 + 5
}

TEST_F(HazardsTest, LoadUseHazardRequiresOneStallBubble) {
    // Load followed immediately by dependent instruction
    // Requires exactly 1 bubble stall
    std::string asm_code = R"(
        ADDI x1, x0, 256
        ADDI x2, x0, 42
        SW   x2, 0(x1)
        LW   x3, 0(x1)
        ADDI x4, x3, 8
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(3), 42u);
    EXPECT_EQ(pipe.registers().read(4), 50u);
    EXPECT_EQ(pipe.get_stats().data_hazard_stalls, 1u);
}

TEST_F(HazardsTest, StoreDataForwarding) {
    // Store data (rs2) is computed right before the store
    // Must forward to SW rs2 correctly!
    std::string asm_code = R"(
        ADDI x1, x0, 256
        ADDI x2, x0, 77
        SW   x2, 0(x1)
        LW   x3, 0(x1)
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = false;
    Pipeline pipe(config);

    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(3), 77u);
    EXPECT_EQ(pipe.data_memory().read_word(256), 77u);
}

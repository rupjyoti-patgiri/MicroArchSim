#include <gtest/gtest.h>
#include "pipeline.h"
#include "assembler.h"
#include "types.h"

using namespace microarchsim;

class MemoryIntegrationTest : public ::testing::Test {
protected:
    Assembler assem;
};

TEST_F(MemoryIntegrationTest, PipelineStallsOnCacheMiss) {
    // Program with a memory load miss
    std::string asm_code = R"(
        ADDI x1, x0, 256
        ADDI x2, x0, 42
        SW   x2, 0(x1)
        LW   x3, 0(x1)
        ADDI x4, x3, 10
        HALT
    )";

    AssemblyResult res = assem.assemble(asm_code);

    SimConfig config;
    config.forwarding_enabled = true;
    config.cache_enabled = true;
    config.cache.cache_size = 128;
    config.cache.line_size = 16;
    config.cache.associativity = 1;
    config.cache.hit_latency = 1;
    config.cache.miss_penalty = 5;

    Pipeline pipe(config);
    pipe.load_program(res.instructions);
    pipe.run();

    EXPECT_EQ(pipe.registers().read(3), 42u);
    EXPECT_EQ(pipe.registers().read(4), 52u);

    Statistics stats = pipe.get_stats();
    // Cache miss penalty introduces memory stall cycles
    EXPECT_GT(stats.memory_stall_cycles, 0u);
    EXPECT_GT(stats.cache_read_misses + stats.cache_write_misses, 0u);
}

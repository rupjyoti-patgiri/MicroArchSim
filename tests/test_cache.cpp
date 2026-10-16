#include <gtest/gtest.h>
#include "cache.h"
#include "memory.h"
#include "types.h"

using namespace microarchsim;

class CacheTest : public ::testing::Test {
protected:
    Memory mem;
    CacheConfig config;

    void SetUp() override {
        // Direct-mapped cache: 64 bytes total, 16-byte lines (4 lines total)
        config.cache_size = 64;
        config.line_size = 16;
        config.associativity = 1;
        config.hit_latency = 1;
        config.miss_penalty = 10;
        config.replacement = ReplacementPolicy::LRU;
        config.write_policy = WritePolicy::WRITE_BACK;
    }
};

TEST_F(CacheTest, CompulsoryMissThenHit) {
    Cache cache(config, mem);

    // Initial read to 0x100 -> Miss
    CacheResult r1 = cache.read(0x100);
    EXPECT_FALSE(r1.hit);
    EXPECT_EQ(r1.latency, 10u);

    // Second read to 0x100 -> Hit
    CacheResult r2 = cache.read(0x100);
    EXPECT_TRUE(r2.hit);
    EXPECT_EQ(r2.latency, 1u);

    // Spatial locality hit within same 16-byte cache line (0x104)
    CacheResult r3 = cache.read(0x104);
    EXPECT_TRUE(r3.hit);
    EXPECT_EQ(r3.latency, 1u);
}

TEST_F(CacheTest, WriteBackPolicyDirtyWriteback) {
    Cache cache(config, mem);

    // Write to 0x100 -> Miss, fills line, updates cache, marks dirty
    CacheResult w1 = cache.write(0x100, 0xCAFEBABE);
    EXPECT_FALSE(w1.hit);

    // Read back from cache -> Hit, returns new value
    CacheResult r1 = cache.read(0x100);
    EXPECT_TRUE(r1.hit);
    EXPECT_EQ(r1.data, 0xCAFEBABEu);

    // Backing memory should NOT have been updated yet (write-back policy)
    EXPECT_NE(mem.read_word(0x100), 0xCAFEBABEu);

    // Now conflict-evict this line:
    // With 4 sets and 16-byte lines:
    // 0x100: index = (0x100 >> 4) & 3 = 0.
    // Address with same index but different tag: 0x100 + 4 * 16 = 0x140.
    CacheResult w2 = cache.write(0x140, 0x12345678);
    EXPECT_FALSE(w2.hit);
    EXPECT_TRUE(w2.writeback_needed);

    // Now backing memory MUST have received the dirty line writeback
    EXPECT_EQ(mem.read_word(0x100), 0xCAFEBABEu);
}

TEST_F(CacheTest, AssociativeLRUReplacement) {
    // 2-way set-associative cache: 64 bytes total, 16-byte lines -> 2 sets, 2 ways
    config.associativity = 2;
    Cache cache(config, mem);

    // Set index for 0x100 (bits [4]): (0x100 >> 4) & 1 = 0
    // Addresses mapping to set 0: 0x100, 0x120, 0x140
    cache.read(0x100); // Way 0: 0x100
    cache.read(0x120); // Way 1: 0x120

    // Touch 0x100 to make it more recently used
    cache.read(0x100);

    // Access 3rd address mapping to set 0 -> evicts LRU (0x120), keeps 0x100
    cache.read(0x140);

    // 0x100 should still HIT
    CacheResult r_hit = cache.read(0x100);
    EXPECT_TRUE(r_hit.hit);

    // 0x120 should MISS (was evicted)
    CacheResult r_miss = cache.read(0x120);
    EXPECT_FALSE(r_miss.hit);
}

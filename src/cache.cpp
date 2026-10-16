/// @file cache.cpp
/// @brief Cache simulator implementation.

#include "cache.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <cassert>
#include <cmath>

namespace microarchsim {

// ============================================================================
// Construction
// ============================================================================

Cache::Cache(const CacheConfig& config, Memory& backing_memory)
    : config_(config)
    , backing_memory_(backing_memory)
{
    config_.validate();

    num_sets_ = config_.num_sets();
    num_ways_ = config_.associativity;
    offset_bits_ = log2(config_.line_size);
    index_bits_ = log2(num_sets_);

    // Initialize cache structure
    sets_.resize(num_sets_);
    for (auto& set : sets_) {
        set.resize(num_ways_, CacheLine(config_.line_size));
    }
}

// ============================================================================
// Address decomposition
// ============================================================================

uint32_t Cache::get_offset(Addr addr) const {
    return addr & (config_.line_size - 1);
}

uint32_t Cache::get_index(Addr addr) const {
    return (addr >> offset_bits_) & (num_sets_ - 1);
}

uint32_t Cache::get_tag(Addr addr) const {
    return addr >> (offset_bits_ + index_bits_);
}

Addr Cache::get_line_addr(Addr addr) const {
    return addr & ~static_cast<Addr>(config_.line_size - 1);
}

// ============================================================================
// Line lookup
// ============================================================================

int Cache::find_line(uint32_t set_index, uint32_t tag) const {
    const auto& set = sets_[set_index];
    for (uint32_t way = 0; way < num_ways_; ++way) {
        if (set[way].valid && set[way].tag == tag) {
            return static_cast<int>(way);
        }
    }
    return -1;
}

// ============================================================================
// Victim selection
// ============================================================================

int Cache::select_victim(uint32_t set_index) const {
    const auto& set = sets_[set_index];

    // First, look for an invalid (empty) line
    for (uint32_t way = 0; way < num_ways_; ++way) {
        if (!set[way].valid) {
            return static_cast<int>(way);
        }
    }

    // All lines valid — use replacement policy
    switch (config_.replacement) {
        case ReplacementPolicy::LRU: {
            // Evict the line with the smallest lru_counter (least recently used)
            int victim = 0;
            uint64_t min_counter = set[0].lru_counter;
            for (uint32_t way = 1; way < num_ways_; ++way) {
                if (set[way].lru_counter < min_counter) {
                    min_counter = set[way].lru_counter;
                    victim = static_cast<int>(way);
                }
            }
            return victim;
        }

        case ReplacementPolicy::FIFO: {
            // Evict the line with the smallest fifo_order (oldest insertion)
            int victim = 0;
            uint64_t min_order = set[0].fifo_order;
            for (uint32_t way = 1; way < num_ways_; ++way) {
                if (set[way].fifo_order < min_order) {
                    min_order = set[way].fifo_order;
                    victim = static_cast<int>(way);
                }
            }
            return victim;
        }

        case ReplacementPolicy::RANDOM: {
            static std::mt19937 rng(42);  // Fixed seed for reproducibility
            std::uniform_int_distribution<int> dist(0, static_cast<int>(num_ways_) - 1);
            return dist(rng);
        }
    }

    return 0;  // Should not reach here
}

// ============================================================================
// Eviction
// ============================================================================

bool Cache::evict(uint32_t set_index, int way) {
    auto& line = sets_[set_index][static_cast<size_t>(way)];
    bool did_writeback = false;

    if (line.valid && line.dirty) {
        // Write dirty line back to backing memory
        Addr line_addr = (line.tag << (offset_bits_ + index_bits_)) | (set_index << offset_bits_);
        for (uint32_t i = 0; i < config_.line_size; ++i) {
            backing_memory_.write_byte(line_addr + i, line.data[i]);
        }
        did_writeback = true;
        stats_.writebacks++;
    }

    line.valid = false;
    line.dirty = false;
    return did_writeback;
}

// ============================================================================
// Line fill
// ============================================================================

void Cache::fill_line(uint32_t set_index, int way, uint32_t tag, Addr line_addr) {
    auto& line = sets_[set_index][static_cast<size_t>(way)];
    line.valid = true;
    line.dirty = false;
    line.tag = tag;
    line.fifo_order = access_counter_;

    // Copy data from backing memory
    for (uint32_t i = 0; i < config_.line_size; ++i) {
        line.data[i] = backing_memory_.read_byte(line_addr + i);
    }
}

// ============================================================================
// Replacement metadata update
// ============================================================================

void Cache::update_on_access(uint32_t set_index, int way) {
    access_counter_++;
    sets_[set_index][static_cast<size_t>(way)].lru_counter = access_counter_;
    // FIFO order is set only on insertion, not on access
}

// ============================================================================
// Line read/write
// ============================================================================

Word Cache::read_from_line(uint32_t set_index, int way, uint32_t offset) const {
    const auto& data = sets_[set_index][static_cast<size_t>(way)].data;
    // Little-endian read, word-aligned within line
    uint32_t aligned_offset = offset & ~3u;
    return static_cast<Word>(data[aligned_offset])
         | (static_cast<Word>(data[aligned_offset + 1]) << 8)
         | (static_cast<Word>(data[aligned_offset + 2]) << 16)
         | (static_cast<Word>(data[aligned_offset + 3]) << 24);
}

void Cache::write_to_line(uint32_t set_index, int way, uint32_t offset, Word value) {
    auto& data = sets_[set_index][static_cast<size_t>(way)].data;
    uint32_t aligned_offset = offset & ~3u;
    data[aligned_offset]     = static_cast<uint8_t>(value & 0xFF);
    data[aligned_offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[aligned_offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[aligned_offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

// ============================================================================
// Read
// ============================================================================

CacheResult Cache::read(Addr addr) {
    uint32_t set_index = get_index(addr);
    uint32_t tag = get_tag(addr);
    uint32_t offset = get_offset(addr);

    int way = find_line(set_index, tag);

    if (way >= 0) {
        // Hit
        stats_.read_hits++;
        update_on_access(set_index, way);
        Word data = read_from_line(set_index, way, offset);
        return {true, data, config_.hit_latency, false};
    }

    // Miss
    stats_.read_misses++;

    // Select victim and evict if necessary
    int victim_way = select_victim(set_index);
    bool wb = evict(set_index, victim_way);

    // Fill from backing memory
    Addr line_addr = get_line_addr(addr);
    fill_line(set_index, victim_way, tag, line_addr);
    update_on_access(set_index, victim_way);

    Word data = read_from_line(set_index, victim_way, offset);
    return {false, data, config_.miss_penalty, wb};
}

// ============================================================================
// Write
// ============================================================================

CacheResult Cache::write(Addr addr, Word value) {
    uint32_t set_index = get_index(addr);
    uint32_t tag = get_tag(addr);
    uint32_t offset = get_offset(addr);

    int way = find_line(set_index, tag);

    if (way >= 0) {
        // Write hit
        stats_.write_hits++;
        update_on_access(set_index, way);
        write_to_line(set_index, way, offset, value);

        if (config_.write_policy == WritePolicy::WRITE_BACK) {
            sets_[set_index][static_cast<size_t>(way)].dirty = true;
        } else {
            // Write-through: also write to backing memory
            backing_memory_.write_word(addr & ~3u, value);
        }
        return {true, 0, config_.hit_latency, false};
    }

    // Write miss
    stats_.write_misses++;

    if (config_.allocate_policy == AllocatePolicy::WRITE_ALLOCATE) {
        // Allocate: fetch line, then write
        int victim_way = select_victim(set_index);
        bool wb = evict(set_index, victim_way);

        Addr line_addr = get_line_addr(addr);
        fill_line(set_index, victim_way, tag, line_addr);
        update_on_access(set_index, victim_way);

        write_to_line(set_index, victim_way, offset, value);

        if (config_.write_policy == WritePolicy::WRITE_BACK) {
            sets_[set_index][static_cast<size_t>(victim_way)].dirty = true;
        } else {
            backing_memory_.write_word(addr & ~3u, value);
        }
        return {false, 0, config_.miss_penalty, wb};
    } else {
        // No-write-allocate: write directly to backing memory
        backing_memory_.write_word(addr & ~3u, value);
        return {false, 0, config_.miss_penalty, false};
    }
}

// ============================================================================
// Flush
// ============================================================================

void Cache::flush() {
    for (uint32_t s = 0; s < num_sets_; ++s) {
        for (uint32_t w = 0; w < num_ways_; ++w) {
            auto& line = sets_[s][w];
            if (line.valid && line.dirty) {
                Addr line_addr = (line.tag << (offset_bits_ + index_bits_)) | (s << offset_bits_);
                for (uint32_t i = 0; i < config_.line_size; ++i) {
                    backing_memory_.write_byte(line_addr + i, line.data[i]);
                }
                line.dirty = false;
                stats_.writebacks++;
            }
        }
    }
}

// ============================================================================
// Reset
// ============================================================================

void Cache::reset_stats() {
    stats_ = CacheStats{};
}

void Cache::reset() {
    reset_stats();
    access_counter_ = 0;
    for (auto& set : sets_) {
        for (auto& line : set) {
            line.valid = false;
            line.dirty = false;
            line.tag = 0;
            line.lru_counter = 0;
            line.fifo_order = 0;
            std::fill(line.data.begin(), line.data.end(), 0);
        }
    }
}

// ============================================================================
// Utility
// ============================================================================

uint32_t Cache::log2(uint32_t v) {
    uint32_t r = 0;
    while (v > 1) {
        v >>= 1;
        r++;
    }
    return r;
}

} // namespace microarchsim

#pragma once

/// @file cache.h
/// @brief Configurable cache simulator with LRU/FIFO/Random replacement,
///        write-back/write-through policies, and performance counters.

#include "types.h"
#include "config.h"
#include "memory.h"
#include <vector>
#include <cstdint>

namespace microarchsim {

/// Result of a cache access.
struct CacheResult {
    bool hit;              ///< Was it a hit?
    Word data;             ///< Data read (for reads)
    uint32_t latency;      ///< Access latency in cycles
    bool writeback_needed; ///< Did this access trigger a dirty eviction?
};

/// Cache statistics (mirrors Statistics cache fields).
struct CacheStats {
    uint64_t read_hits    = 0;
    uint64_t read_misses  = 0;
    uint64_t write_hits   = 0;
    uint64_t write_misses = 0;
    uint64_t writebacks   = 0;

    uint64_t total_accesses() const {
        return read_hits + read_misses + write_hits + write_misses;
    }

    double hit_rate() const {
        uint64_t total = total_accesses();
        return total > 0 ? static_cast<double>(read_hits + write_hits) / static_cast<double>(total) : 0.0;
    }
};

/// A single cache line.
struct CacheLine {
    bool valid    = false;
    bool dirty    = false;
    uint32_t tag  = 0;
    uint64_t lru_counter = 0;  ///< For LRU: higher = more recently used
    uint64_t fifo_order  = 0;  ///< For FIFO: insertion order
    std::vector<uint8_t> data;

    CacheLine() = default;
    explicit CacheLine(uint32_t line_size) : data(line_size, 0) {}
};

/// Configurable cache simulator.
class Cache {
public:
    /// Construct with configuration and backing memory.
    Cache(const CacheConfig& config, Memory& backing_memory);

    /// Read a word from the cache. May fill from backing memory on miss.
    CacheResult read(Addr addr);

    /// Write a word to the cache. Behavior depends on write/allocate policy.
    CacheResult write(Addr addr, Word value);

    /// Flush all dirty lines to backing memory.
    void flush();

    /// Get current cache statistics.
    CacheStats get_stats() const { return stats_; }

    /// Reset statistics counters.
    void reset_stats();

    /// Reset cache contents and statistics.
    void reset();

    /// Get configuration.
    const CacheConfig& config() const { return config_; }

private:
    CacheConfig config_;
    Memory& backing_memory_;
    CacheStats stats_;

    std::vector<std::vector<CacheLine>> sets_;  ///< sets_[set_index][way]

    uint32_t num_sets_;
    uint32_t num_ways_;

    // Bit field widths
    uint32_t offset_bits_;
    uint32_t index_bits_;

    // Global counter for LRU/FIFO tracking
    uint64_t access_counter_ = 0;

    /// Extract the block offset from an address.
    uint32_t get_offset(Addr addr) const;

    /// Extract the set index from an address.
    uint32_t get_index(Addr addr) const;

    /// Extract the tag from an address.
    uint32_t get_tag(Addr addr) const;

    /// Get the base address of the cache line containing addr.
    Addr get_line_addr(Addr addr) const;

    /// Find a cache line in the given set. Returns way index or -1.
    int find_line(uint32_t set_index, uint32_t tag) const;

    /// Select a victim line for eviction. Returns way index.
    int select_victim(uint32_t set_index) const;

    /// Evict a line, writing back if dirty. Returns whether writeback occurred.
    bool evict(uint32_t set_index, int way);

    /// Fill a cache line from backing memory.
    void fill_line(uint32_t set_index, int way, uint32_t tag, Addr line_addr);

    /// Update replacement metadata on access.
    void update_on_access(uint32_t set_index, int way);

    /// Read a word from a cache line.
    Word read_from_line(uint32_t set_index, int way, uint32_t offset) const;

    /// Write a word to a cache line.
    void write_to_line(uint32_t set_index, int way, uint32_t offset, Word value);

    /// Log2 helper.
    static uint32_t log2(uint32_t v);
};

} // namespace microarchsim

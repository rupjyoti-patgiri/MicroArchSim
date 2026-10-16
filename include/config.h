#pragma once

/// @file config.h
/// @brief Simulator configuration — all tunable parameters in one place.

#include "types.h"
#include <string>
#include <stdexcept>

namespace microarchsim {

// ============================================================================
// Cache configuration
// ============================================================================

struct CacheConfig {
    uint32_t cache_size    = 1024;    ///< Total cache size in bytes (must be power of 2, >= 64)
    uint32_t line_size     = 16;      ///< Cache line size in bytes (must be power of 2, >= 4)
    uint32_t associativity = 1;       ///< 1 = direct-mapped; N = N-way set-associative
    uint32_t hit_latency   = 1;       ///< Hit latency in cycles (>= 1)
    uint32_t miss_penalty  = 10;      ///< Miss penalty in cycles (>= 1)

    ReplacementPolicy replacement = ReplacementPolicy::LRU;
    WritePolicy       write_policy = WritePolicy::WRITE_BACK;
    AllocatePolicy    allocate_policy = AllocatePolicy::WRITE_ALLOCATE;

    /// Validate configuration constraints. Throws on invalid config.
    void validate() const {
        auto is_power_of_2 = [](uint32_t v) { return v > 0 && (v & (v - 1)) == 0; };

        if (!is_power_of_2(cache_size) || cache_size < 64)
            throw SimulatorError("Cache size must be a power of 2 and >= 64 bytes");
        if (!is_power_of_2(line_size) || line_size < 4)
            throw SimulatorError("Cache line size must be a power of 2 and >= 4 bytes");
        if (line_size > cache_size)
            throw SimulatorError("Cache line size cannot exceed cache size");
        if (associativity == 0)
            throw SimulatorError("Associativity must be >= 1");

        uint32_t num_lines = cache_size / line_size;
        if (associativity > num_lines)
            throw SimulatorError("Associativity cannot exceed total number of cache lines");
        if (num_lines % associativity != 0)
            throw SimulatorError("Number of cache lines must be divisible by associativity");
        if (hit_latency < 1)
            throw SimulatorError("Hit latency must be >= 1");
        if (miss_penalty < 1)
            throw SimulatorError("Miss penalty must be >= 1");

        // Write-through with no-write-allocate is valid.
        // Write-back with no-write-allocate is unusual but we allow it.
    }

    /// Number of sets in the cache.
    uint32_t num_sets() const { return cache_size / (line_size * associativity); }

    /// Total number of cache lines.
    uint32_t num_lines() const { return cache_size / line_size; }
};

// ============================================================================
// Simulator configuration
// ============================================================================

struct SimConfig {
    // Execution mode
    SimMode mode = SimMode::PIPELINE;

    // Pipeline configuration
    bool forwarding_enabled = true;   ///< Enable data forwarding (EX→EX, MEM→EX)
    bool branch_predict_taken = false; ///< false = predict not-taken (default)

    // Memory configuration
    uint32_t memory_size = 1 << 20;   ///< Total memory size in bytes (default 1 MiB)
    bool unified_memory = false;       ///< true = shared I/D memory (structural hazard)

    // Cache configuration
    bool cache_enabled = true;         ///< Enable data cache
    CacheConfig cache;

    // Output configuration
    bool trace_enabled = false;        ///< Print cycle-by-cycle pipeline state
    bool dump_registers = false;       ///< Print final register state
    bool dump_memory = false;          ///< Print non-zero data memory contents
    std::string stats_csv_path;        ///< Path to export CSV statistics (empty = no export)

    // Input
    std::string program_path;          ///< Path to assembly program file

    /// Validate all configuration fields.
    void validate() const {
        if (program_path.empty())
            throw SimulatorError("No program file specified");
        if (memory_size < 4096)
            throw SimulatorError("Memory size must be >= 4096 bytes");
        if (cache_enabled)
            cache.validate();
    }
};

} // namespace microarchsim

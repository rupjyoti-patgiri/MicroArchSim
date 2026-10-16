#pragma once

/// @file statistics.h
/// @brief Performance counters and metrics collection for the simulator.

#include "types.h"
#include <string>
#include <ostream>
#include <cstdint>

namespace microarchsim {

/// Raw performance counters collected during simulation.
struct Statistics {
    // Cycle counts
    uint64_t total_cycles           = 0;
    uint64_t productive_cycles      = 0;
    uint64_t data_hazard_stalls     = 0;
    uint64_t control_flush_cycles   = 0;
    uint64_t memory_stall_cycles    = 0;
    uint64_t drain_cycles           = 0;

    // Instruction counts
    uint64_t instructions_fetched   = 0;
    uint64_t instructions_issued    = 0;   ///< Entered ID stage
    uint64_t instructions_completed = 0;   ///< Exited MEM stage
    uint64_t instructions_retired   = 0;   ///< Completed WB stage

    // Hazard events
    uint64_t load_use_stalls        = 0;   ///< Number of load-use stall events
    uint64_t branch_flushes         = 0;   ///< Number of branch/jump flush events
    uint64_t instructions_flushed   = 0;   ///< Total instructions squashed by flushes

    // Cache statistics
    uint64_t cache_read_hits        = 0;
    uint64_t cache_read_misses      = 0;
    uint64_t cache_write_hits       = 0;
    uint64_t cache_write_misses     = 0;
    uint64_t cache_writebacks       = 0;   ///< Dirty line evictions

    // Derived metrics (computed, not accumulated)

    /// Cycles per retired instruction. Returns 0.0 if no instructions retired.
    double cpi() const {
        return instructions_retired > 0
            ? static_cast<double>(total_cycles) / static_cast<double>(instructions_retired)
            : 0.0;
    }

    /// Instructions per cycle. Returns 0.0 if no cycles.
    double ipc() const {
        return total_cycles > 0
            ? static_cast<double>(instructions_retired) / static_cast<double>(total_cycles)
            : 0.0;
    }

    /// Pipeline utilization (fraction of cycles that were productive).
    double pipeline_utilization() const {
        return total_cycles > 0
            ? static_cast<double>(instructions_retired) / static_cast<double>(total_cycles)
            : 0.0;
    }

    /// Total cache accesses (reads + writes).
    uint64_t total_cache_accesses() const {
        return cache_read_hits + cache_read_misses + cache_write_hits + cache_write_misses;
    }

    /// Cache hit rate. Returns 0.0 if no accesses.
    double cache_hit_rate() const {
        uint64_t total = total_cache_accesses();
        return total > 0
            ? static_cast<double>(cache_read_hits + cache_write_hits) / static_cast<double>(total)
            : 0.0;
    }

    /// Cache miss rate. Returns 0.0 if no accesses.
    double cache_miss_rate() const {
        return 1.0 - cache_hit_rate();
    }

    /// Total memory traffic in words (misses that go to main memory + writebacks).
    uint64_t memory_traffic_words() const {
        return (cache_read_misses + cache_write_misses) + cache_writebacks;
    }

    /// Print a formatted summary to the output stream.
    void print_summary(std::ostream& os) const;

    /// Export statistics as a CSV row. If header is true, print the header line first.
    void export_csv(std::ostream& os, bool header = false,
                    const std::string& program_name = "",
                    const std::string& config_label = "") const;
};

/// Collector that accumulates statistics during simulation.
class StatisticsCollector {
public:
    /// Record a cycle of a given type.
    void record_cycle(CycleType type);

    /// Record an instruction fetch.
    void record_fetch();

    /// Record an instruction issue (entered decode).
    void record_issue();

    /// Record an instruction completion (exited MEM).
    void record_completion();

    /// Record an instruction retirement (completed WB).
    void record_retirement();

    /// Record a load-use stall event.
    void record_load_use_stall();

    /// Record a branch/jump flush event with number of squashed instructions.
    void record_branch_flush(uint64_t instructions_squashed);

    /// Record a cache read hit.
    void record_cache_read_hit();

    /// Record a cache read miss.
    void record_cache_read_miss();

    /// Record a cache write hit.
    void record_cache_write_hit();

    /// Record a cache write miss.
    void record_cache_write_miss();

    /// Record a dirty-line writeback.
    void record_writeback();

    /// Get the accumulated statistics.
    const Statistics& get() const { return stats_; }

    /// Reset all counters.
    void reset();

private:
    Statistics stats_;
};

} // namespace microarchsim

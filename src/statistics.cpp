/// @file statistics.cpp
/// @brief Statistics collection and reporting implementation.

#include "statistics.h"
#include <iomanip>
#include <sstream>

namespace microarchsim {

// ============================================================================
// StatisticsCollector
// ============================================================================

void StatisticsCollector::record_cycle(CycleType type) {
    stats_.total_cycles++;
    switch (type) {
        case CycleType::PRODUCTIVE:        stats_.productive_cycles++;     break;
        case CycleType::DATA_HAZARD_STALL: stats_.data_hazard_stalls++;    break;
        case CycleType::CONTROL_FLUSH:     stats_.control_flush_cycles++;  break;
        case CycleType::MEMORY_STALL:      stats_.memory_stall_cycles++;   break;
        case CycleType::DRAIN:             stats_.drain_cycles++;          break;
    }
}

void StatisticsCollector::record_fetch()      { stats_.instructions_fetched++; }
void StatisticsCollector::record_issue()      { stats_.instructions_issued++; }
void StatisticsCollector::record_completion() { stats_.instructions_completed++; }
void StatisticsCollector::record_retirement() { stats_.instructions_retired++; }

void StatisticsCollector::record_load_use_stall() {
    stats_.load_use_stalls++;
}

void StatisticsCollector::record_branch_flush(uint64_t instructions_squashed) {
    stats_.branch_flushes++;
    stats_.instructions_flushed += instructions_squashed;
}

void StatisticsCollector::record_cache_read_hit()   { stats_.cache_read_hits++; }
void StatisticsCollector::record_cache_read_miss()  { stats_.cache_read_misses++; }
void StatisticsCollector::record_cache_write_hit()  { stats_.cache_write_hits++; }
void StatisticsCollector::record_cache_write_miss() { stats_.cache_write_misses++; }
void StatisticsCollector::record_writeback()        { stats_.cache_writebacks++; }

void StatisticsCollector::reset() {
    stats_ = Statistics{};
}

// ============================================================================
// Statistics printing
// ============================================================================

void Statistics::print_summary(std::ostream& os) const {
    os << "\n==================== Performance Summary ====================\n";

    os << "\n--- Cycle Breakdown ---\n";
    os << "  Total cycles:          " << total_cycles << "\n";
    os << "  Productive cycles:     " << productive_cycles << "\n";
    os << "  Data hazard stalls:    " << data_hazard_stalls << "\n";
    os << "  Control flush cycles:  " << control_flush_cycles << "\n";
    os << "  Memory stall cycles:   " << memory_stall_cycles << "\n";
    os << "  Drain cycles:          " << drain_cycles << "\n";

    os << "\n--- Instruction Counts ---\n";
    os << "  Fetched:               " << instructions_fetched << "\n";
    os << "  Issued:                " << instructions_issued << "\n";
    os << "  Completed:             " << instructions_completed << "\n";
    os << "  Retired:               " << instructions_retired << "\n";
    os << "  Flushed:               " << instructions_flushed << "\n";

    os << "\n--- Performance Metrics ---\n";
    os << "  CPI:                   " << std::fixed << std::setprecision(3) << cpi() << "\n";
    os << "  IPC:                   " << std::fixed << std::setprecision(3) << ipc() << "\n";
    os << "  Pipeline utilization:  " << std::fixed << std::setprecision(1)
       << (pipeline_utilization() * 100.0) << "%\n";

    os << "\n--- Hazard Events ---\n";
    os << "  Load-use stalls:       " << load_use_stalls << "\n";
    os << "  Branch/jump flushes:   " << branch_flushes << "\n";

    if (total_cache_accesses() > 0) {
        os << "\n--- Cache Statistics ---\n";
        os << "  Read hits:             " << cache_read_hits << "\n";
        os << "  Read misses:           " << cache_read_misses << "\n";
        os << "  Write hits:            " << cache_write_hits << "\n";
        os << "  Write misses:          " << cache_write_misses << "\n";
        os << "  Hit rate:              " << std::fixed << std::setprecision(1)
           << (cache_hit_rate() * 100.0) << "%\n";
        os << "  Miss rate:             " << std::fixed << std::setprecision(1)
           << (cache_miss_rate() * 100.0) << "%\n";
        os << "  Dirty writebacks:      " << cache_writebacks << "\n";
        os << "  Memory traffic (words):" << memory_traffic_words() << "\n";
    }

    os << "=============================================================\n";
}

void Statistics::export_csv(std::ostream& os, bool header,
                            const std::string& program_name,
                            const std::string& config_label) const {
    if (header) {
        os << "program,config,total_cycles,instructions_retired,"
           << "cpi,ipc,pipeline_utilization,"
           << "productive_cycles,data_hazard_stalls,control_flush_cycles,"
           << "memory_stall_cycles,drain_cycles,"
           << "instructions_fetched,instructions_flushed,"
           << "load_use_stalls,branch_flushes,"
           << "cache_read_hits,cache_read_misses,"
           << "cache_write_hits,cache_write_misses,"
           << "cache_hit_rate,cache_miss_rate,cache_writebacks\n";
    }

    os << program_name << "," << config_label << ","
       << total_cycles << "," << instructions_retired << ","
       << std::fixed << std::setprecision(4) << cpi() << ","
       << std::fixed << std::setprecision(4) << ipc() << ","
       << std::fixed << std::setprecision(4) << pipeline_utilization() << ","
       << productive_cycles << "," << data_hazard_stalls << ","
       << control_flush_cycles << "," << memory_stall_cycles << ","
       << drain_cycles << ","
       << instructions_fetched << "," << instructions_flushed << ","
       << load_use_stalls << "," << branch_flushes << ","
       << cache_read_hits << "," << cache_read_misses << ","
       << cache_write_hits << "," << cache_write_misses << ","
       << std::fixed << std::setprecision(4) << cache_hit_rate() << ","
       << std::fixed << std::setprecision(4) << cache_miss_rate() << ","
       << cache_writebacks << "\n";
}

} // namespace microarchsim

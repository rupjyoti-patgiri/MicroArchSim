#pragma once

/// @file pipeline.h
/// @brief Five-stage pipelined processor engine.

#include "types.h"
#include "config.h"
#include "instruction.h"
#include "register_file.h"
#include "memory.h"
#include "cache.h"
#include "pipeline_registers.h"
#include "hazard_unit.h"
#include "statistics.h"
#include <vector>
#include <ostream>
#include <memory>

namespace microarchsim {

/// Five-stage in-order pipelined processor.
class Pipeline {
public:
    /// Construct with simulator configuration.
    explicit Pipeline(const SimConfig& config);

    /// Load instruction words into instruction memory.
    void load_program(const std::vector<Word>& instructions, Addr base = INSTRUCTION_BASE);

    /// Load data words into data memory.
    void load_data(const std::vector<Word>& data, Addr base = DATA_BASE);

    /// Run until HALT + pipeline drain.
    void run();

    /// Execute one clock cycle. Returns false when pipeline is fully drained.
    bool step();

    /// Get current pipeline state.
    const PipelineState& state() const { return current_; }

    /// Get the register file.
    const RegisterFile& registers() const { return reg_file_; }

    /// Get the data memory.
    const Memory& data_memory() const { return data_memory_; }

    /// Get accumulated statistics.
    Statistics get_stats() const { return stats_.get(); }

    /// Get current cycle count.
    uint64_t cycle_count() const { return cycle_count_; }

    /// Has the pipeline fully drained after HALT?
    bool is_finished() const { return finished_; }

    /// Reset all state.
    void reset();

    /// Set trace output stream (nullptr to disable).
    void set_trace_output(std::ostream* os) { trace_os_ = os; }

private:
    SimConfig config_;

    // Processor state
    RegisterFile reg_file_;
    Memory instruction_memory_;
    Memory data_memory_;
    std::unique_ptr<Cache> data_cache_;

    // Pipeline registers (double-buffered)
    PipelineState current_;
    PipelineState next_;

    // Hazard unit
    HazardUnit hazard_unit_;

    // Statistics
    StatisticsCollector stats_;
    uint64_t cycle_count_ = 0;

    // Control state
    bool halt_fetched_ = false;    ///< HALT has been fetched, stop fetching
    bool finished_     = false;    ///< Pipeline fully drained
    bool stall_        = false;    ///< Stall IF and ID this cycle
    bool branch_flush_ = false;    ///< Flush IF/ID and ID/EX this cycle (branch in EX)
    bool jump_flush_   = false;    ///< Flush IF/ID this cycle (jump in ID)
    Addr flush_target_ = 0;        ///< PC target on flush

    // Memory stall
    uint32_t mem_stall_remaining_ = 0;  ///< Remaining cycles for memory stall

    // Trace
    std::ostream* trace_os_ = nullptr;

    // Stage implementations (processed in reverse order: WB → MEM → EX → ID → IF)
    void do_writeback();
    void do_memory();
    void do_execute();
    void do_decode();
    void do_fetch();

    /// Commit next_ to current_ at end of cycle.
    void commit_state();

    /// Check if pipeline is empty (all stages invalid).
    bool pipeline_empty() const;

    /// Print cycle trace to trace output.
    void print_trace() const;

    /// Classify this cycle for statistics.
    CycleType classify_cycle() const;
};

} // namespace microarchsim

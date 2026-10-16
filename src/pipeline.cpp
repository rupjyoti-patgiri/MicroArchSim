/// @file pipeline.cpp
/// @brief Five-stage pipelined processor implementation.
///
/// Stages are processed in reverse order (WB → MEM → EX → ID → IF) each cycle.
/// A double-buffered state model (current_ / next_) prevents read-after-write
/// races within a single cycle. All updates to next_ are committed atomically
/// at the end of each cycle.

#include "pipeline.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace microarchsim {

// ============================================================================
// Construction
// ============================================================================

Pipeline::Pipeline(const SimConfig& config)
    : config_(config)
    , instruction_memory_(config.memory_size)
    , data_memory_(config.memory_size)
    , hazard_unit_(config.forwarding_enabled)
{
    if (config.cache_enabled) {
        data_cache_ = std::make_unique<Cache>(config.cache, data_memory_);
    }
    if (config.trace_enabled) {
        trace_os_ = &std::cout;
    }
}

void Pipeline::load_program(const std::vector<Word>& instructions, Addr base) {
    instruction_memory_.load_program(instructions, base);
}

void Pipeline::load_data(const std::vector<Word>& data, Addr base) {
    data_memory_.load_data(data, base);
}

// ============================================================================
// Run
// ============================================================================

void Pipeline::run() {
    while (!finished_) {
        step();
    }
}

bool Pipeline::step() {
    if (finished_) return false;

    cycle_count_++;

    // Initialize next state from current state
    next_ = current_;

    // Reset per-cycle control signals
    stall_ = false;
    branch_flush_ = false;
    jump_flush_ = false;
    flush_target_ = 0;

    // If we're in a memory stall, count down and don't process stages
    if (mem_stall_remaining_ > 0) {
        mem_stall_remaining_--;
        stats_.record_cycle(CycleType::MEMORY_STALL);

        if (mem_stall_remaining_ == 0) {
            // Memory stall complete — the MEM stage will re-execute next cycle
            // and get a cache hit (line is now filled)
        }

        // During memory stall, WB can still complete if it has a valid instruction
        // But for simplicity with blocking cache, we freeze everything
        return true;
    }

    // --- Hazard detection (before stage execution) ---

    // Check for load-use hazard: instruction in ID/EX is a load, and
    // instruction being decoded in IF/ID reads from the load's rd
    if (current_.if_id.valid && current_.id_ex.valid) {
        // Decode the instruction in IF/ID to get its rs1, rs2
        DecodedInstruction decoded = decode(current_.if_id.instruction_word);
        if (hazard_unit_.detect_load_use_hazard(current_.id_ex, current_.if_id,
                                                  decoded.rs1, decoded.rs2)) {
            stall_ = true;
            stats_.record_load_use_stall();
        }
    }

    // --- Process stages in reverse order ---
    do_writeback();
    do_memory();
    do_execute();
    do_decode();
    do_fetch();

    // --- Handle flush (overrides stall for IF and ID) ---
    if (branch_flush_) {
        // Branch misprediction resolved in EX stage: flush both younger stages (IF/ID and ID/EX)
        next_.if_id.clear();
        next_.id_ex.clear();
        next_.pc = flush_target_;
        halt_fetched_ = false; // In case HALT was speculatively fetched down the fall-through path!

        uint64_t flushed = 0;
        if (current_.if_id.valid) flushed++;
        if (current_.id_ex.valid) flushed++;
        if (flushed > 0) {
            stats_.record_branch_flush(flushed);
        }
    } else if (jump_flush_) {
        // Jump (JAL) resolved in ID stage: flush only the instruction fetched behind it (IF/ID)
        // The JAL instruction itself is in ID/EX (next_.id_ex) and MUST proceed through EX/MEM/WB!
        next_.if_id.clear();
        next_.pc = flush_target_;
        halt_fetched_ = false; // In case HALT was speculatively fetched down the fall-through path!

        uint64_t flushed = 0;
        if (current_.if_id.valid) flushed++;
        if (flushed > 0) {
            stats_.record_branch_flush(flushed);
        }
    }

    // --- Handle stall ---
    if (stall_ && !branch_flush_ && !jump_flush_) {
        // Freeze IF/ID (keep the same instruction)
        next_.if_id = current_.if_id;
        // Freeze PC
        next_.pc = current_.pc;
        // Insert bubble into ID/EX
        next_.id_ex.clear();
    }

    // --- Commit state ---
    commit_state();

    // --- Classify cycle for statistics ---
    if (mem_stall_remaining_ > 0) {
        // Already counted above
    } else if (stall_ && !branch_flush_ && !jump_flush_) {
        stats_.record_cycle(CycleType::DATA_HAZARD_STALL);
    } else if (branch_flush_ || jump_flush_) {
        stats_.record_cycle(CycleType::CONTROL_FLUSH);
    } else if (halt_fetched_ && !pipeline_empty()) {
        stats_.record_cycle(CycleType::DRAIN);
    } else {
        stats_.record_cycle(CycleType::PRODUCTIVE);
    }

    // --- Print trace ---
    if (trace_os_) {
        print_trace();
    }

    // --- Check if pipeline is fully drained ---
    if (halt_fetched_ && pipeline_empty()) {
        finished_ = true;
    }

    return !finished_;
}

// ============================================================================
// WB Stage
// ============================================================================

void Pipeline::do_writeback() {
    const auto& mem_wb = current_.mem_wb;

    if (!mem_wb.valid) return;

    // Write result to register file
    if (mem_wb.reg_write && mem_wb.rd != 0) {
        Word write_val = mem_wb.write_data();
        reg_file_.write(mem_wb.rd, write_val);
    }

    stats_.record_retirement();

    // Check for HALT completing WB
    if (mem_wb.is_halt) {
        // HALT has retired — but other instructions might still be in-flight
        // The pipeline will drain naturally
    }
}

// ============================================================================
// MEM Stage
// ============================================================================

void Pipeline::do_memory() {
    const auto& ex_mem = current_.ex_mem;
    auto& mem_wb_next = next_.mem_wb;

    if (!ex_mem.valid) {
        mem_wb_next.clear();
        return;
    }

    mem_wb_next.id = ex_mem.id;
    mem_wb_next.rd = ex_mem.rd;
    mem_wb_next.reg_write = ex_mem.reg_write;
    mem_wb_next.mem_to_reg = ex_mem.mem_to_reg;
    mem_wb_next.alu_result = ex_mem.alu_result;
    mem_wb_next.is_halt = ex_mem.is_halt;
    mem_wb_next.valid = true;

    // Memory read (LW)
    if (ex_mem.mem_read) {
        Addr addr = ex_mem.alu_result;
        if (data_cache_) {
            CacheResult result = data_cache_->read(addr);
            mem_wb_next.mem_data = result.data;

            if (!result.hit) {
                stats_.record_cache_read_miss();
                if (result.writeback_needed) stats_.record_writeback();
                // Stall pipeline for miss penalty - 1 (1 cycle is this cycle)
                if (result.latency > 1) {
                    mem_stall_remaining_ = result.latency - 1;
                }
            } else {
                stats_.record_cache_read_hit();
                if (result.latency > 1) {
                    mem_stall_remaining_ = result.latency - 1;
                }
            }
        } else {
            mem_wb_next.mem_data = data_memory_.read_word(addr);
        }

        stats_.record_completion();
    }
    // Memory write (SW)
    else if (ex_mem.mem_write) {
        Addr addr = ex_mem.alu_result;
        Word data = ex_mem.rs2_val;

        if (data_cache_) {
            CacheResult result = data_cache_->write(addr, data);

            if (!result.hit) {
                stats_.record_cache_write_miss();
                if (result.writeback_needed) stats_.record_writeback();
                if (result.latency > 1) {
                    mem_stall_remaining_ = result.latency - 1;
                }
            } else {
                stats_.record_cache_write_hit();
                if (result.latency > 1) {
                    mem_stall_remaining_ = result.latency - 1;
                }
            }
        } else {
            data_memory_.write_word(addr, data);
        }

        stats_.record_completion();
    } else {
        stats_.record_completion();
    }
}

// ============================================================================
// EX Stage
// ============================================================================

void Pipeline::do_execute() {
    const auto& id_ex = current_.id_ex;
    auto& ex_mem_next = next_.ex_mem;

    if (!id_ex.valid) {
        ex_mem_next.clear();
        return;
    }

    // Get operand values (apply forwarding)
    Word op_a = id_ex.rs1_val;
    Word op_b = id_ex.rs2_val;

    ForwardingResult fwd_a = hazard_unit_.get_forward_a(id_ex, current_.ex_mem, current_.mem_wb);
    ForwardingResult fwd_b = hazard_unit_.get_forward_b(id_ex, current_.ex_mem, current_.mem_wb);

    if (fwd_a.source != ForwardSource::NONE) op_a = fwd_a.value;
    if (fwd_b.source != ForwardSource::NONE) op_b = fwd_b.value;

    // ALU computation
    Word alu_result = 0;
    SWord op_a_signed = static_cast<SWord>(op_a);
    Word imm_unsigned = static_cast<Word>(id_ex.immediate);

    switch (id_ex.alu_op) {
        case ALUOp::ADD:
            if (id_ex.mem_read || id_ex.mem_write) {
                // Load/Store: address = rs1 + immediate
                alu_result = op_a + imm_unsigned;
            } else if (id_ex.id == InstructionID::ADDI) {
                alu_result = op_a + imm_unsigned;
            } else {
                // ADD: rd = rs1 + rs2
                alu_result = op_a + op_b;
            }
            break;

        case ALUOp::SUB:
            if (id_ex.is_branch) {
                // Branch comparison uses subtraction
                alu_result = op_a - op_b;
            } else {
                alu_result = op_a - op_b;
            }
            break;

        case ALUOp::AND:
            if (id_ex.id == InstructionID::ANDI) {
                alu_result = op_a & imm_unsigned;
            } else {
                alu_result = op_a & op_b;
            }
            break;

        case ALUOp::OR:
            alu_result = op_a | op_b;
            break;

        case ALUOp::XOR:
            alu_result = op_a ^ op_b;
            break;

        case ALUOp::SLT:
            if (id_ex.id == InstructionID::SLTI) {
                alu_result = (op_a_signed < id_ex.immediate) ? 1 : 0;
            } else {
                alu_result = (op_a_signed < static_cast<SWord>(op_b)) ? 1 : 0;
            }
            break;

        case ALUOp::PASS_B:
            // LUI: pass the immediate through
            alu_result = imm_unsigned;
            break;

        case ALUOp::ADD_PC:
            // AUIPC: PC + immediate
            alu_result = id_ex.pc + imm_unsigned;
            break;

        case ALUOp::NOP:
            break;
    }

    // Branch resolution
    bool branch_taken = false;
    Addr branch_target = 0;

    if (id_ex.is_branch) {
        // BEQ: taken if rs1 == rs2 (alu_result == 0 after SUB)
        // BNE: taken if rs1 != rs2 (alu_result != 0 after SUB)
        if (id_ex.id == InstructionID::BEQ) {
            branch_taken = (op_a == op_b);
        } else if (id_ex.id == InstructionID::BNE) {
            branch_taken = (op_a != op_b);
        }

        if (branch_taken) {
            branch_target = id_ex.pc + static_cast<Word>(id_ex.immediate);
            branch_flush_ = true;
            flush_target_ = branch_target;
        }
    }

    // JAL was already handled in ID stage for the flush,
    // but we compute the link address here
    if (id_ex.is_jump) {
        alu_result = id_ex.pc + 4;  // Link address (return address)
    }

    // Populate EX/MEM register
    ex_mem_next.pc = id_ex.pc;
    ex_mem_next.id = id_ex.id;
    ex_mem_next.alu_result = alu_result;
    ex_mem_next.rs2_val = op_b;  // Forwarded store data
    ex_mem_next.rd = id_ex.rd;
    ex_mem_next.reg_write = id_ex.reg_write;
    ex_mem_next.mem_read = id_ex.mem_read;
    ex_mem_next.mem_write = id_ex.mem_write;
    ex_mem_next.mem_to_reg = id_ex.mem_to_reg;
    ex_mem_next.is_halt = id_ex.is_halt;
    ex_mem_next.branch_taken = branch_taken;
    ex_mem_next.branch_target = branch_target;
    ex_mem_next.valid = true;
}

// ============================================================================
// ID Stage
// ============================================================================

void Pipeline::do_decode() {
    const auto& if_id = current_.if_id;
    auto& id_ex_next = next_.id_ex;

    // If stalling, we already handle this in step() — don't overwrite
    if (stall_) return;

    if (!if_id.valid) {
        id_ex_next.clear();
        return;
    }

    // Decode instruction
    DecodedInstruction instr = decode(if_id.instruction_word);

    stats_.record_issue();

    // Read register values
    Word rs1_val = reg_file_.read(instr.rs1);
    Word rs2_val = reg_file_.read(instr.rs2);

    // Populate ID/EX register
    id_ex_next.pc = if_id.pc;
    id_ex_next.id = instr.id;
    id_ex_next.rs1 = instr.rs1;
    id_ex_next.rs2 = instr.rs2;
    id_ex_next.rd = instr.rd;
    id_ex_next.rs1_val = rs1_val;
    id_ex_next.rs2_val = rs2_val;
    id_ex_next.immediate = instr.immediate;
    id_ex_next.alu_op = instr.alu_op;
    id_ex_next.reg_write = instr.reg_write;
    id_ex_next.mem_read = instr.mem_read;
    id_ex_next.mem_write = instr.mem_write;
    id_ex_next.mem_to_reg = instr.mem_to_reg;
    id_ex_next.is_branch = instr.is_branch;
    id_ex_next.is_jump = instr.is_jump;
    id_ex_next.is_halt = instr.is_halt;
    id_ex_next.valid = true;

    // JAL: resolve in ID stage (1-cycle penalty)
    // We know the target from the immediate, so flush IF/ID and redirect PC
    if (instr.is_jump) {
        Addr target = if_id.pc + static_cast<Word>(instr.immediate);
        jump_flush_ = true;
        flush_target_ = target;
        // Note: The JAL instruction itself continues through EX/MEM/WB normally
        // to write PC+4 to rd. Only the instruction fetched AFTER the JAL is flushed.
    }
}

// ============================================================================
// IF Stage
// ============================================================================

void Pipeline::do_fetch() {
    // If stalling, keep current IF/ID and PC
    if (stall_) return;

    auto& if_id_next = next_.if_id;

    if (halt_fetched_) {
        if_id_next.clear();
        return;
    }

    // Fetch instruction from instruction memory
    Word instruction_word = instruction_memory_.read_word(current_.pc);

    if_id_next.instruction_word = instruction_word;
    if_id_next.pc = current_.pc;
    if_id_next.next_pc = current_.pc + 4;
    if_id_next.valid = true;

    stats_.record_fetch();

    // Check for HALT
    if (instruction_word == HALT_INSTRUCTION) {
        halt_fetched_ = true;
    }

    // Update PC (default: PC + 4, may be overridden by flush)
    next_.pc = current_.pc + 4;
}

// ============================================================================
// State commit
// ============================================================================

void Pipeline::commit_state() {
    current_ = next_;
}

bool Pipeline::pipeline_empty() const {
    return !current_.if_id.valid &&
           !current_.id_ex.valid &&
           !current_.ex_mem.valid &&
           !current_.mem_wb.valid;
}

// ============================================================================
// Trace output
// ============================================================================

void Pipeline::print_trace() const {
    if (!trace_os_) return;

    auto& os = *trace_os_;
    os << "Cycle " << std::setw(4) << cycle_count_ << " | PC=0x" << std::hex
       << std::setw(8) << std::setfill('0') << current_.pc << std::dec
       << std::setfill(' ');

    auto stage_str = [](const auto& reg, const char* name) -> std::string {
        if (!reg.valid) return std::string(name) + ":[---]";
        return std::string(name) + ":[" + instruction_name(reg.id) + "]";
    };

    // For IF/ID, decode the instruction to get its name
    std::string if_str;
    if (current_.if_id.valid) {
        try {
            auto dec = decode(current_.if_id.instruction_word);
            if_str = "IF:[" + instruction_name(dec.id) + "]";
        } catch (...) {
            if_str = "IF:[???]";
        }
    } else {
        if_str = "IF:[---]";
    }

    os << " | " << std::setw(14) << if_str
       << " | " << std::setw(14) << stage_str(current_.id_ex, "ID")
       << " | " << std::setw(14) << stage_str(current_.ex_mem, "EX")
       << " | " << std::setw(14) << stage_str(current_.mem_wb, "MEM");

    if (stall_) os << " [STALL]";
    if (branch_flush_ || jump_flush_) os << " [FLUSH→0x" << std::hex << flush_target_ << std::dec << "]";
    if (mem_stall_remaining_ > 0) os << " [MEM_STALL:" << mem_stall_remaining_ << "]";

    os << "\n";
}

CycleType Pipeline::classify_cycle() const {
    if (mem_stall_remaining_ > 0) return CycleType::MEMORY_STALL;
    if (stall_) return CycleType::DATA_HAZARD_STALL;
    if (branch_flush_ || jump_flush_) return CycleType::CONTROL_FLUSH;
    if (halt_fetched_) return CycleType::DRAIN;
    return CycleType::PRODUCTIVE;
}

// ============================================================================
// Reset
// ============================================================================

void Pipeline::reset() {
    reg_file_.reset();
    instruction_memory_.reset();
    data_memory_.reset();
    if (data_cache_) data_cache_->reset();

    current_.clear();
    next_.clear();

    stats_.reset();
    cycle_count_ = 0;
    halt_fetched_ = false;
    finished_ = false;
    stall_ = false;
    branch_flush_ = false;
    jump_flush_ = false;
    flush_target_ = 0;
    mem_stall_remaining_ = 0;
}

} // namespace microarchsim

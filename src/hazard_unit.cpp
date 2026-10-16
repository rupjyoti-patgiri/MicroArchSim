/// @file hazard_unit.cpp
/// @brief Hazard detection and forwarding implementation.

#include "hazard_unit.h"

namespace microarchsim {

HazardUnit::HazardUnit(bool forwarding_enabled)
    : forwarding_enabled_(forwarding_enabled) {}

// ============================================================================
// Load-use hazard detection
// ============================================================================

bool HazardUnit::detect_load_use_hazard(const IDEX_Register& id_ex,
                                          const IFID_Register& /*if_id_next*/,
                                          Reg decode_rs1, Reg decode_rs2) const {
    // A load-use hazard exists when:
    // 1. The instruction in EX is a load (mem_read == true)
    // 2. Its destination register (rd) matches rs1 or rs2 of the instruction in ID
    // 3. The destination is not x0
    //
    // When forwarding is disabled, ANY RAW hazard causes a stall, but load-use
    // is the critical case even with forwarding because the data isn't available
    // until the end of the MEM stage.

    if (!id_ex.valid) return false;

    if (forwarding_enabled_) {
        // With forwarding: only load-use requires a stall
        if (id_ex.mem_read && id_ex.rd != 0) {
            if (id_ex.rd == decode_rs1 || id_ex.rd == decode_rs2) {
                return true;
            }
        }
    } else {
        // Without forwarding: any RAW hazard from EX stage requires stall
        if (id_ex.reg_write && id_ex.rd != 0) {
            if (id_ex.rd == decode_rs1 || id_ex.rd == decode_rs2) {
                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// Forwarding
// ============================================================================

ForwardingResult HazardUnit::check_forwarding(Reg rs,
                                                const EXMEM_Register& ex_mem,
                                                const MEMWB_Register& mem_wb) const {
    if (!forwarding_enabled_ || rs == 0) {
        return {ForwardSource::NONE, 0};
    }

    // Priority: EX/MEM has the more recent value (younger instruction)
    // EX/MEM forwarding
    if (ex_mem.valid && ex_mem.reg_write && ex_mem.rd != 0 && ex_mem.rd == rs) {
        // Cannot forward from a load in EX/MEM — that's a load-use hazard
        // which should have been detected and stalled already.
        // By the time we get here, EX/MEM either has the ALU result (non-load)
        // or the load has completed (stall was inserted, now MEM/WB has the value).
        if (!ex_mem.mem_to_reg) {
            return {ForwardSource::EX_MEM, ex_mem.alu_result};
        }
        // If it IS a load in EX/MEM, we shouldn't get here because the stall
        // should have prevented this instruction from advancing. But as a safety
        // measure, fall through to MEM/WB.
    }

    // MEM/WB forwarding
    if (mem_wb.valid && mem_wb.reg_write && mem_wb.rd != 0 && mem_wb.rd == rs) {
        return {ForwardSource::MEM_WB, mem_wb.write_data()};
    }

    return {ForwardSource::NONE, 0};
}

ForwardingResult HazardUnit::get_forward_a(const IDEX_Register& id_ex,
                                             const EXMEM_Register& ex_mem,
                                             const MEMWB_Register& mem_wb) const {
    return check_forwarding(id_ex.rs1, ex_mem, mem_wb);
}

ForwardingResult HazardUnit::get_forward_b(const IDEX_Register& id_ex,
                                             const EXMEM_Register& ex_mem,
                                             const MEMWB_Register& mem_wb) const {
    return check_forwarding(id_ex.rs2, ex_mem, mem_wb);
}

} // namespace microarchsim

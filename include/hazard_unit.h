#pragma once

/// @file hazard_unit.h
/// @brief Hazard detection and data forwarding logic.

#include "types.h"
#include "pipeline_registers.h"

namespace microarchsim {

/// Forwarding decision for one operand.
struct ForwardingResult {
    ForwardSource source = ForwardSource::NONE;
    Word value = 0;  ///< The forwarded value (only valid if source != NONE)
};

/// Hazard detection and forwarding unit.
class HazardUnit {
public:
    /// @param forwarding_enabled If false, all RAW hazards cause stalls.
    explicit HazardUnit(bool forwarding_enabled = true);

    /// Check for load-use hazard: the EX-stage instruction is a load whose
    /// rd matches rs1 or rs2 of the instruction in ID.
    /// Returns true if a stall is needed.
    bool detect_load_use_hazard(const IDEX_Register& id_ex,
                                 const IFID_Register& if_id_next,
                                 Reg decode_rs1, Reg decode_rs2) const;

    /// Determine forwarding for operand A (rs1 of ID/EX instruction).
    ForwardingResult get_forward_a(const IDEX_Register& id_ex,
                                    const EXMEM_Register& ex_mem,
                                    const MEMWB_Register& mem_wb) const;

    /// Determine forwarding for operand B (rs2 of ID/EX instruction).
    ForwardingResult get_forward_b(const IDEX_Register& id_ex,
                                    const EXMEM_Register& ex_mem,
                                    const MEMWB_Register& mem_wb) const;

    /// Check if forwarding is enabled.
    bool is_forwarding_enabled() const { return forwarding_enabled_; }

private:
    bool forwarding_enabled_;

    /// Internal forwarding check for a single register.
    ForwardingResult check_forwarding(Reg rs,
                                       const EXMEM_Register& ex_mem,
                                       const MEMWB_Register& mem_wb) const;
};

} // namespace microarchsim

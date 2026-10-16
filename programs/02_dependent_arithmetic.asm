# Workload 2: Consecutive Dependent Instructions (RAW Hazards)
# Tests EX/MEM -> EX forwarding across tight dependency chains.
# With forwarding: 0 stalls, CPI ~ 1.0.
# Without forwarding: 2 stall cycles per dependency.

    ADDI x1, x0, 1
    ADD  x2, x1, x1
    ADD  x3, x2, x2
    ADD  x4, x3, x3
    ADD  x5, x4, x4
    ADD  x6, x5, x5
    ADD  x7, x6, x6
    ADD  x8, x7, x7
    ADD  x9, x8, x8
    ADD  x10, x9, x9
    HALT

# Workload 1: Independent Arithmetic Instructions
# Tests peak throughput without data hazards or branch flushes.
# Expected: CPI ~ 1.0 (after 4-cycle pipeline fill), zero stalls.

    ADDI x1, x0, 1
    ADDI x2, x0, 2
    ADDI x3, x0, 3
    ADDI x4, x0, 4
    ADDI x5, x0, 5
    ADDI x6, x0, 6
    ADDI x7, x0, 7
    ADDI x8, x0, 8
    ADDI x9, x0, 9
    ADDI x10, x0, 10
    ADDI x11, x0, 11
    ADDI x12, x0, 12
    ADDI x13, x0, 13
    ADDI x14, x0, 14
    ADDI x15, x0, 15
    ADDI x16, x0, 16
    HALT

# Workload 10: Working Set Larger than Configured Cache
# For a 128-byte cache (8 lines of 16 bytes), traverses 256 bytes (16 lines).
# Continuously induces capacity and conflict evictions across repeated iterations.

    ADDI x1, x0, 256     # Base address
    ADDI x2, x0, 3       # 3 full passes over array

pass_loop:
    ADDI x3, x0, 16      # 16 cache-line steps (16 lines * 16 bytes = 256 bytes)
    ADDI x4, x0, 0       # offset

elem_loop:
    ADD  x5, x1, x4
    LW   x6, 0(x5)
    ADDI x6, x6, 1
    SW   x6, 0(x5)
    ADDI x4, x4, 16      # jump to next line
    ADDI x3, x3, -1
    BNE  x3, x0, elem_loop

    ADDI x2, x2, -1
    BNE  x2, x0, pass_loop

    HALT

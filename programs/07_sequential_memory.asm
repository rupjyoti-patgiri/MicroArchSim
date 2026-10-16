# Workload 7: Sequential Memory Access
# Sequential 4-byte accesses traversing contiguous words.
# Tests spatial locality within cache lines (1 compulsory miss per line, subsequent hits).

    ADDI x1, x0, 256     # Base address
    ADDI x2, x0, 16      # 16 elements
    ADDI x3, x0, 0       # Offset counter
    ADDI x4, x0, 4       # Stride = 4 bytes

write_loop:
    ADD  x5, x1, x3      # Address = base + offset
    SW   x3, 0(x5)       # Write index value
    ADD  x3, x3, x4      # offset += 4
    ADDI x2, x2, -1      # counter--
    BNE  x2, x0, write_loop

    # Read back sequentially
    ADDI x2, x0, 16
    ADDI x3, x0, 0
    ADDI x6, x0, 0       # Sum

read_loop:
    ADD  x5, x1, x3
    LW   x7, 0(x5)
    ADD  x6, x6, x7
    ADD  x3, x3, x4
    ADDI x2, x2, -1
    BNE  x2, x0, read_loop

    HALT

# Workload 8: Strided Memory Access
# Accesses memory with 64-byte stride, defeating line spatial locality.
# Each access lands in a different cache line, triggering compulsory misses.

    ADDI x1, x0, 256     # Base address
    ADDI x2, x0, 8       # 8 strided accesses
    ADDI x3, x0, 0       # Offset counter
    ADDI x4, x0, 64      # Large stride = 64 bytes

stride_loop:
    ADD  x5, x1, x3
    SW   x2, 0(x5)
    LW   x6, 0(x5)
    ADD  x3, x3, x4
    ADDI x2, x2, -1
    BNE  x2, x0, stride_loop

    HALT

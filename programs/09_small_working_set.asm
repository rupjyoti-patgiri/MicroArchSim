# Workload 9: Repeated Access to Small Working Set
# Fits entirely within cache (4 words = 16 bytes).
# Tests temporal locality: initial compulsory miss followed by 100% cache hits across loop iterations.

    ADDI x1, x0, 256     # Base address
    ADDI x2, x0, 10      # 10 outer iterations

outer_loop:
    # Access 4 contiguous words repeatedly
    LW   x3, 0(x1)
    ADDI x3, x3, 1
    SW   x3, 0(x1)

    LW   x4, 4(x1)
    ADDI x4, x4, 1
    SW   x4, 4(x1)

    ADDI x2, x2, -1
    BNE  x2, x0, outer_loop

    HALT

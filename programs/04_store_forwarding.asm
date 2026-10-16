# Workload 4: Store Operations and Store-Data Forwarding
# Tests forwarding into SW data operand (rs2) from preceding ALU instructions.

    ADDI x1, x0, 256
    
    # Store-data forward directly from previous ALU result
    ADDI x2, x0, 111
    SW   x2, 0(x1)

    ADDI x3, x2, 222
    SW   x3, 4(x1)

    ADDI x4, x3, 333
    SW   x4, 8(x1)

    # Verify stores by loading back
    LW   x5, 0(x1)
    LW   x6, 4(x1)
    LW   x7, 8(x1)

    HALT

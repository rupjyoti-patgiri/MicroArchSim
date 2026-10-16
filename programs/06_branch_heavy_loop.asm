# Workload 6: Branch-Heavy Loop Workload
# Tight loop executing 20 iterations to benchmark branch penalty on CPI.

    ADDI x1, x0, 20      # Loop counter = 20
    ADDI x2, x0, 0       # Accumulator = 0
    ADDI x3, x0, 1       # Decrement step

loop:
    ADD  x2, x2, x1      # acc += counter
    SUB  x1, x1, x3      # counter--
    BNE  x1, x0, loop    # Repeat until counter == 0

    HALT

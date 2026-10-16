# Workload 5: Taken and Not-Taken Branches
# Tests static not-taken prediction:
# Taken branch incurs 2-cycle penalty (flushes IF/ID and ID/EX).
# Not-taken branch incurs 0 penalty cycles (pipeline proceeds uninterrupted).

    ADDI x1, x0, 10
    ADDI x2, x0, 20
    ADDI x3, x0, 10

    # Branch 1: NOT TAKEN (x1 != x2). Zero flush penalty.
    BEQ  x1, x2, never_target
    ADDI x4, x0, 100

    # Branch 2: TAKEN (x1 == x3). Two flush cycles.
    BEQ  x1, x3, taken_target
    ADDI x4, x0, 999   # Flushed instruction

never_target:
    ADDI x4, x0, 888   # Never reached

taken_target:
    ADDI x5, x0, 200
    HALT

# Workload 3: Load-Use Hazard Dependencies
# Tests hardware stall bubble insertion when an ALU instruction immediately consumes load data.
# Expected: Exactly 1 stall cycle per load-use pair.

    ADDI x1, x0, 256
    ADDI x2, x0, 10
    SW   x2, 0(x1)
    SW   x2, 4(x1)
    SW   x2, 8(x1)

    # Pair 1: Load followed immediately by dependent ADD
    LW   x3, 0(x1)
    ADD  x4, x3, x2

    # Pair 2: Load followed immediately by dependent SUB
    LW   x5, 4(x1)
    SUB  x6, x5, x2

    # Pair 3: Load followed immediately by dependent AND
    LW   x7, 8(x1)
    AND  x8, x7, x2

    HALT

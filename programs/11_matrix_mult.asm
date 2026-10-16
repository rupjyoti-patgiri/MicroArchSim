# Workload 11: 2x2 Matrix Multiplication (C = A * B)
# Matrix elements stored at memory addresses:
# A: base A (256): A[0,0], A[0,1], A[1,0], A[1,1]
# B: base B (272): B[0,0], B[0,1], B[1,0], B[1,1]
# C: base C (288): C[0,0], C[0,1], C[1,0], C[1,1]

    # Initialize A = [[1, 2], [3, 4]]
    ADDI x1, x0, 256     # base A
    ADDI x2, x0, 1
    SW   x2, 0(x1)       # A[0,0] = 1
    ADDI x2, x0, 2
    SW   x2, 4(x1)       # A[0,1] = 2
    ADDI x2, x0, 3
    SW   x2, 8(x1)       # A[1,0] = 3
    ADDI x2, x0, 4
    SW   x2, 12(x1)      # A[1,1] = 4

    # Initialize B = [[5, 6], [7, 8]]
    ADDI x10, x0, 272    # base B
    ADDI x2, x0, 5
    SW   x2, 0(x10)      # B[0,0] = 5
    ADDI x2, x0, 6
    SW   x2, 4(x10)      # B[0,1] = 6
    ADDI x2, x0, 7
    SW   x2, 8(x10)      # B[1,0] = 7
    ADDI x2, x0, 8
    SW   x2, 12(x10)     # B[1,1] = 8

    ADDI x20, x0, 288    # base C

    # Load matrix elements into registers
    LW   x11, 0(x1)      # a00 = 1
    LW   x12, 4(x1)      # a01 = 2
    LW   x13, 8(x1)      # a10 = 3
    LW   x14, 12(x1)     # a11 = 4

    LW   x15, 0(x10)     # b00 = 5
    LW   x16, 4(x10)     # b01 = 6
    LW   x17, 8(x10)     # b10 = 7
    LW   x18, 12(x10)    # b11 = 8

    # Note: RV32I-Edu has addition, so we compute multiplication via repeated addition
    # C[0,0] = a00 * b00 + a01 * b10 = 1 * 5 + 2 * 7 = 5 + 14 = 19
    ADDI x21, x0, 0
    ADD  x21, x21, x15   # 1 * 5
    ADD  x21, x21, x17   # + 7
    ADD  x21, x21, x17   # + 7 = 19
    SW   x21, 0(x20)

    # C[0,1] = a00 * b01 + a01 * b11 = 1 * 6 + 2 * 8 = 6 + 16 = 22
    ADDI x22, x0, 0
    ADD  x22, x22, x16   # 1 * 6
    ADD  x22, x22, x18   # + 8
    ADD  x22, x22, x18   # + 8 = 22
    SW   x22, 4(x20)

    # C[1,0] = a10 * b00 + a11 * b10 = 3 * 5 + 4 * 7 = 15 + 28 = 43
    ADDI x23, x0, 0
    ADD  x23, x23, x15
    ADD  x23, x23, x15
    ADD  x23, x23, x15   # 3 * 5 = 15
    ADD  x23, x23, x17
    ADD  x23, x23, x17
    ADD  x23, x23, x17
    ADD  x23, x23, x17   # + 4 * 7 = 43
    SW   x23, 8(x20)

    # C[1,1] = a10 * b01 + a11 * b11 = 3 * 6 + 4 * 8 = 18 + 32 = 50
    ADDI x24, x0, 0
    ADD  x24, x24, x16
    ADD  x24, x24, x16
    ADD  x24, x24, x16   # 3 * 6 = 18
    ADD  x24, x24, x18
    ADD  x24, x24, x18
    ADD  x24, x24, x18
    ADD  x24, x24, x18   # + 4 * 8 = 50
    SW   x24, 12(x20)

    HALT

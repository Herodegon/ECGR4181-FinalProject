main:                                   # Addr 0x0
    lui     t0, %hi(1024)               # Load upper immediate for ARRAY_A
    lw      t0, %lo(1024)(t0)           # Load ARRAY_A base address
    lui     t1, %hi(ARRAY_B_ADDR)       # Load upper immediate for ARRAY_B
    lw      t1, %lo(ARRAY_B_ADDR)(t1)   # Load ARRAY_B base address
    lui     t2, %hi(ARRAY_C_ADDR)       # Load upper immediate for ARRAY_C
    lw      t2, %lo(ARRAY_C_ADDR)(t2)   # Load ARRAY_C base address

    li      t3, 0                       # Initialize loop counter i = 0
    j       .LBB0_1

.LBB0_1:                                # Loop start
    li      t4, 256                     # Load upper bound for loop
    bge     t3, t4, .LBB0_4             # If i >= 256, exit loop

    slli    t5, t3, 2                   # i * 4 (for float indexing)
    add     a0, t0, t5                  # a0 = ARRAY_A + (i * 4)
    flw     ft0, 0(a0)                  # Load ARRAY_A[i] into ft0

    add     a1, t1, t5                  # a1 = ARRAY_B + (i * 4)
    flw     ft1, 0(a1)                  # Load ARRAY_B[i] into ft1

    fadd.s  ft0, ft0, ft1               # ft0 = ARRAY_A[i] + ARRAY_B[i]

    add     a2, t2, t5                  # a2 = ARRAY_C + (i * 4)
    fsw     ft0, 0(a2)                  # Store result in ARRAY_C[i]

    addi    t3, t3, 1                   # i++

    j       .LBB0_1                     # Repeat loop

.LBB0_4:                                # Loop end
    li      a0, 0                       # Return value 0
    ret

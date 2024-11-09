    .data
ARRAY_A_ADDR: .word 0x0400       # Base address of ARRAY_A
ARRAY_B_ADDR: .word 0x0800       # Base address of ARRAY_B
ARRAY_C_ADDR: .word 0x0C00       # Base address of ARRAY_C

    .text
    .globl main

main:
    # Set up the stack frame
    addi sp, sp, -16            # Allocate stack space
    sw ra, 12(sp)               # Save return address
    sw s0, 8(sp)                # Save frame pointer

    # Initialize the frame pointer
    addi s0, sp, 16

    # Load base addresses of arrays
    la t0, ARRAY_A_ADDR         # Load address of ARRAY_A
    lw t0, 0(t0)                # Get ARRAY_A base address into t0
    la t1, ARRAY_B_ADDR         # Load address of ARRAY_B
    lw t1, 0(t1)                # Get ARRAY_B base address into t1
    la t2, ARRAY_C_ADDR         # Load address of ARRAY_C
    lw t2, 0(t2)                # Get ARRAY_C base address into t2

    # Set loop counter
    li t3, 0                    # Initialize i = 0
    li t4, 256                  # Loop limit (ARRAY size)

loop:
    bge t3, t4, end_loop        # if i >= 256, exit loop

    # Load elements from ARRAY_A and ARRAY_B
    flw f0, 0(t0)               # Load ARRAY_A[i] into f0
    flw f1, 0(t1)               # Load ARRAY_B[i] into f1

    # Perform floating-point addition
    fadd.s f2, f0, f1           # f2 = f0 + f1

    # Store result in ARRAY_C
    fsw f2, 0(t2)               # Store result in ARRAY_C[i]

    # Increment pointers and counter
    addi t0, t0, 4              # Move to next element in ARRAY_A
    addi t1, t1, 4              # Move to next element in ARRAY_B
    addi t2, t2, 4              # Move to next element in ARRAY_C
    addi t3, t3, 1              # i++

    j loop                      # Repeat the loop

end_loop:
    # Restore the stack and return
    lw ra, 12(sp)               # Restore return address
    lw s0, 8(sp)                # Restore frame pointer
    addi sp, sp, 16             # Deallocate stack space
    jr ra                       # Return from main

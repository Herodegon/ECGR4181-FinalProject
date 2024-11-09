main:                       # Addr 0x0
	addi sp, sp, -16        # Allocates space on the stack
	sw ra, 12(sp)           # Saves return address
	sw s0, 8(sp)            # Saves s0 register
	addi s0, sp, 16         # Sets up a frame pointer
	mv a0, zero             # Zeroes out a0 (used later as a counter)
	sw a0, -12(s0)          # Initializes a variable at -12(s0) to 0
	sw a0, -16(s0)          # Initializes the loop counter at -16(s0) to 0

.LBB0_1:                                # Addr 0x20
	lw a0, -16(s0)          # Loads loop counter (i) from -16(s0)
	addi a1, zero, 255      # Sets upper limit (255) in a1
	blt a1, a0, .LBB0_4     # If i > 255, jump to loop end

.LBB0_2:                                # Addr 0x30
	lui a0, %hi(1024)       # Loads the high part of ARRAY_A address
	lw a0, %lo(1024)(a0)    # Loads base address of ARRAY_A into a0
	lw a1, -16(s0)          # Loads i from -16(s0)
	slli a1, a1, 2          # Multiplies i by 4 to get byte offset
	add a0, a0, a1          # Adds offset to base address of ARRAY_A
	flw ft0, 0(a0)          # Loads ARRAY_A[i] into ft0

	lui a0, %hi(2048)       # Loads high part of ARRAY_B address
	lw a0, %lo(2048)(a0)    # Loads base address of ARRAY_B into a0
	add a0, a0, a1          # Adds offset to base address of ARRAY_B
	flw ft1, 0(a0)          # Loads ARRAY_B[i] into ft1

	fadd.s ft0, ft0, ft1    # Adds ARRAY_A[i] and ARRAY_B[i] into ft0

	lui a0, %hi(3072)       # Loads high part of ARRAY_C address
	lw a0, %lo(3072)(a0)    # Loads base address of ARRAY_C into a0
	add a0, a0, a1          # Adds offset to base address of ARRAY_C
	fsw ft0, 0(a0)          # Stores result in ARRAY_C[i]

.LBB0_3:                                # Addr 0x70
	lw a0, -16(s0)          # Reloads loop counter (i)
	addi a0, a0, 1          # Increments i by 1
	sw a0, -16(s0)          # Stores updated i
	j .LBB0_1               # Jumps back to the start of the loop

.LBB0_4:				# Addr 0x80
	lw a0, -12(s0)          # Loads another variable at -12(s0) (if needed)
	lw s0, 8(sp)            # Restores s0 register
	lw ra, 12(sp)           # Restores return address
	addi sp, sp, 16         # Deallocates stack space
	ret                     # Returns from the function

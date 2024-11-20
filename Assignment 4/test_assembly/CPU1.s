main:                       # 0 Addr 0x0
	addi sp, sp, -16        # 8 Allocates space on the stack
	sw ra, 12(sp)           # 12 Saves return address
	sw s0, 8(sp)            # 16 Saves s0 register
	addi s0, sp, 16         # 20 Sets up a frame pointer
	mv a0, zero             # 24 Zeroes out a0 (used later as a counter) | addi a0 zero 0 is fine
	sw a0, -12(s0)          # 28 Initializes a variable at -12(s0) to 0
	sw a0, -16(s0)          # 32 Initializes the loop counter at -16(s0) to 0
# 7
.LBB0_1:                    # 36 Addr 0x20 | jal x0, 32 
	lw a0, -16(s0)          # 40 Loads loop counter (i) from -16(s0) 
	addi a1, zero, 255      # 44 Sets upper limit (255) in a1
	blt a1, a0, .LBB0_4     # 48 If i > 255, jump to loop end | blt x11 x10 0 changed to blt x11 x10 128 0x08A5C063 FIX LATER TO ADD TO PC
# 11
.LBB0_2:                    # 52 Addr 0x30 | jal x0 16 0x0100006F changed to 0x0300006F jal x0 48
	mv a0, zero		        # 56 Loads the high part of ARRAY_A address # ISSUE | lui x10 0
	addi a0, a0, 1024     	# 60 14 Loads base address of ARRAY_A into a0
	lw a1, -16(s0)          # 64 Loads i from -16(s0)
	slli a1, a1, 2          # 68 Multiplies i by 4 to get byte offset
	add a0, a0, a1          # 72 Adds offset to base address of ARRAY_A
	flw ft0, 0(a0)          # 76 Loads ARRAY_A[i] into ft0
# 18
	mv a0, zero       		# 80 Loads high part of ARRAY_B address # ISSUE lui a0, 0
	addi a0, a0, 2048     	# 84 Loads base address of ARRAY_B into a0 # ADD TWO HERE
	add a0, a0, a1          # 88 Adds offset to base address of ARRAY_B
	flw ft1, 0(a0)          # 92 Loads ARRAY_B[i] into ft1
# 22
	fsub.s ft0, ft0, ft1    # 96 Adds ARRAY_A[i] and ARRAY_B[i] into ft0 ISSUE ""
# 23
	mv a0, zero       		# 100 Loads high part of ARRAY_C address # ISSUE lui a0, 0
	addi a0, a0, 3072     	# 84 Loads base address of ARRAY_B into a0 # ADD TWO HERE
	addi a0, a0, 1024
	add a0, a0, a1          # 108 Adds offset to base address of ARRAY_C
	fsw ft0, 0(a0)          # 112 Stores result in ARRAY_C[i] ISSUE ra, 0(a0)
# 29
.LBB0_3:                    # 116 Addr 0x70 | jal zero, 64 0x0400006F changed to 0x0700006F jal zero, 112
	lw a0, -16(s0)          # 120 Reloads loop counter (i) # ISSUE lui a0, 0 | 0xC0052503 |
	addi a0, a0, 1          # 124 Increments i by 1		   # ISSUE add a0, a0, a1
	sw a0, -16(s0)          # 128 Stores updated i
	j .LBB0_1               # 132 Jumps back to the start of the loop | jal zero, -80 0xFB1FF06F changed to 0x0200006F

.LBB0_4:					# Addr 0x80
	lw a0, -12(s0)          # Loads another variable at -12(s0) (if needed)
	lw s0, 8(sp)            # Restores s0 register
	lw ra, 12(sp)           # Restores return address
	addi sp, sp, 16         # Deallocates stack space
	ret                     # Returns from the function

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define RAM_SIZE 0xC00
#define NUM_REGISTERS 32
#define CPU_CYCLE_TICKS 10
#define RAM_LATENCY_TICKS 20
#define RV32I_LATENCY_TICKS 10
#define RV32F_LATENCY_TICKS 50

// Integer and Floating Point Register Banks
uint32_t int_regs[NUM_REGISTERS];
float fp_regs[NUM_REGISTERS];

// Program Counter
uint32_t pc = 0;

// RAM (byte-addressable)
uint8_t ram[RAM_SIZE];

// Simulation tick counter
uint32_t sim_ticks = 0;

// Pipeline stage variables
uint32_t fetched_instruction;
uint32_t decoded_instruction;
bool mem_access = false;
uint32_t execute_result;

// Initialize RAM with given memory map specifications by reading from the binary file
void init_ram(const char *filename) {
    FILE *file = fopen(filename, "rb"); // Open file in binary mode
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Read binary instructions into RAM
    size_t read_size = fread(ram, 1, RAM_SIZE, file);
    if (read_size != RAM_SIZE) {
        printf("Warning: Read %zu bytes, expected %d bytes\n", read_size, RAM_SIZE);
    }

    fclose(file);
    printf("RAM initialized with instructions from %s.\n", filename);
}

// Fetch stage
void fetch() {
    if (pc < RAM_SIZE - 4) {
        fetched_instruction = *(uint32_t*)&ram[pc];
        pc += 4;
        printf("Fetched instruction: 0x%08X at PC: 0x%08X\n", fetched_instruction, pc);
    }
}

// Decode stage: Decode the opcode from the fetched instruction
void decode() {
    decoded_instruction = fetched_instruction; // Simple decode for now
    printf("Decoded instruction: 0x%08X\n", decoded_instruction);
}

// Execute stage: Perform specific operations based on the opcode
void execute() {
    uint32_t opcode = decoded_instruction & 0x7F; // Get last 7 bits for opcode

    switch (opcode) {
        case 0x33: // Example for ADD
            int_regs[1] = int_regs[2] + int_regs[3]; // Modify based on your instruction format
            sim_ticks += RV32I_LATENCY_TICKS;
            printf("Executed ADD instruction: Result = %u\n", int_regs[1]);
            break;

        case 0x53: // Example for FADD.S
            fp_regs[1] = fp_regs[2] + fp_regs[3]; // Modify based on your instruction format
            sim_ticks += RV32F_LATENCY_TICKS;
            printf("Executed FADD.S instruction: Result = %f\n", fp_regs[1]);
            break;

        case 0x03: // Example for LOAD
            mem_access = true;
            printf("Load instruction decoded. Preparing memory access.\n");
            break;

        default:
            printf("Unknown or unimplemented instruction opcode: 0x%02X\n", opcode);
            break;
    }
}

// Memory stage: Handle memory accesses if required
void memory() {
    if (mem_access) {
        sim_ticks += RAM_LATENCY_TICKS;
        printf("Memory access complete with latency.\n");
        mem_access = false;
    }
}

// Write-back stage: Write results to registers (simulated)
void write_back() {
    printf("Write-back stage complete.\n");
}

// Simulate the CPU pipeline
void simulate_pipeline() {
    while (pc < 0x094) { // Run until the end of loaded instructions
        write_back();
        memory();
        execute();
        decode();
        fetch();

        // Simulate CPU cycle and ticks
        sim_ticks += CPU_CYCLE_TICKS;
        printf("Simulation ticks: %u\n", sim_ticks);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program.bin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_ram(argv[1]); // Pass the binary file name to init_ram
    simulate_pipeline();
    return 0;
}

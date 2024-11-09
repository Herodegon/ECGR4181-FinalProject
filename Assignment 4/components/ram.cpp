// ram.cpp
#include "ram.h"
#include <cstdlib>    // for rand()
#include <cstring>    // for std::memcpy

// Constructor: Initializes RAM and sets up specific memory regions
RAM::RAM() {
    std::memset(memory, 0, RAM_SIZE);   // Initialize RAM with zeroes
    initializeMemoryRegions();          // Initialize arrays with random FP32 values
}

// Read a 32-bit word from RAM with simulated latency
uint32_t RAM::read(uint32_t address) {
    if (address + 4 > RAM_SIZE) {
        throw std::out_of_range("RAM read out of bounds.");
    }
    uint32_t value;
    std::memcpy(&value, &memory[address], sizeof(value));
    return value;
}

// Write a 32-bit word to RAM with simulated latency
void RAM::write(uint32_t address, uint32_t value) {
    if (address + 4 > RAM_SIZE) {
        throw std::out_of_range("RAM write out of bounds.");
    }
    std::memcpy(&memory[address], &value, sizeof(value));
}

// Print memory contents for debugging
void RAM::print(uint32_t start, uint32_t end) const {
    for (uint32_t i = start; i < end; i += 4) {
        uint32_t value;
        std::memcpy(&value, &memory[i], sizeof(value));
        std::cout << "Address 0x" << std::hex << i << ": 0x" << value << std::dec << '\n';
    }
}

// Initialize specific memory regions as per specifications
void RAM::initializeMemoryRegions() {
    // Initialize arrays ARRAY_A (0x400-0x7FF) and ARRAY_B (0x800-0xBFF) with random FP32 values
    for (uint32_t i = 0x400; i < 0xC00; i += 4) {
        float randomValue = static_cast<float>(rand()) / RAND_MAX;
        std::memcpy(&memory[i], &randomValue, sizeof(float));
    }
}

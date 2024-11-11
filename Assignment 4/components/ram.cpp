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
        std::cout << value << ' ';  // Print only the decimal value
    }
    std::cout << '\n';
}

// Initialize specific memory regions as per specifications
void RAM::initializeMemoryRegions() {
    // Initialize memory from 0x400 to 0xBFF with sequential uint32_t values starting from 1
    uint32_t value = 1;
    std::cout << std::endl;
    for (uint32_t address = 0x400; address < 0x7FF; address += 4) {
        write(address, value);
        value += 1;
    }
    value = 1;
    for (uint32_t address = 0x800; address < 0xBFF; address += 4) {
        write(address, value);
        value += 1;
    }
}

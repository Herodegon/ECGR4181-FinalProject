// ram.h
#ifndef RAM_H
#define RAM_H

#include <cstdint>
#include <iostream>

class RAM {
public:
    static const uint32_t RAM_SIZE = 0x13FF;  // Size of RAM
    static const int READ_LATENCY = 20;       // RAM read latency in simulation ticks
    static const int WRITE_LATENCY = 20;      // RAM write latency in simulation ticks

    RAM();

    // Read a 32-bit word from RAM with simulated latency
    uint32_t read(uint32_t address);

    // Write a 32-bit word to RAM with simulated latency
    void write(uint32_t address, uint32_t value);

    // Print memory contents for debugging
    void print(uint32_t start, uint32_t end) const;

private:
    uint8_t memory[RAM_SIZE];  // RAM storage array

    // Initialize specific memory regions as per specifications
    void initializeMemoryRegions();
};

#endif // RAM_H

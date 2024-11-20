// ram.h
#ifndef RAM_H
#define RAM_H

#include <cstdint>
#include <iostream>
#include <vector>
#include <cstdlib>    // for rand()
#include <cstring>    // for std::memcpy
#include <climits>
#include <ctime>
#include <map>

struct AddressDelay {
    uint32_t load;
    uint32_t store;
};

class RAM {
public:
    static const uint32_t RAM_SIZE = 0x1400;  // Size of RAM
    static const int READ_LATENCY = 20;       // RAM read latency in simulation ticks
    static const int WRITE_LATENCY = 20;      // RAM write latency in simulation ticks

    RAM();

    // Read a 32-bit word from RAM with simulated latency
    std::vector<uint32_t> read(uint32_t address, bool bypass);

    // Write a 32-bit word to RAM with simulated latency
    std::vector<uint32_t> write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass);

    // Print memory contents for debugging
    void print(uint32_t start, uint32_t end) const;

    void printMath();

private:
    uint8_t memory[RAM_SIZE];  // RAM storage array

    // Map to keep track of delays per address
    std::map<uint32_t, AddressDelay> addressDelays;

    int read_write_delay;

    // Initialize specific memory regions as per specifications
    void initializeMemoryRegions();
};

#endif // RAM_H

#ifndef MEMBUS_H
#define MEMBUS_H

#include "ram.h"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <set>

class Membus {
public:
    // Constructor: Takes a reference to a RAM instance
    Membus(RAM& ramInstance);

    // Write to memory and return a vector of results
    std::vector<uint32_t> write(int core_id, uint32_t address, uint32_t value, uint32_t added_delay, bool bypass);

    // Read from memory and return a vector of results
    std::vector<uint32_t> read(int core_id, uint32_t address, bool bypass);

private:
    RAM& ram;  // Reference to RAM object for memory operations
    std::unordered_map<uint32_t, std::tuple<int, uint32_t, uint32_t>> addressInUse;   // Map to track which core is using which address
};

#endif // MEMBUS_H

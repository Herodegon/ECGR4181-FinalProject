#include "membus.h"
#include <iostream>

// Constructor to initialize Membus with a reference to RAM
Membus::Membus(RAM& ramInstance) : ram(ramInstance) {}

// Write method to interact with RAM
std::vector<uint32_t> Membus::write(int core_id, uint32_t address, uint32_t value, uint32_t added_delay, bool bypass) {
    // Check if the address is already in use by another core
    auto it = addressInUse.find(address);
    if (it != addressInUse.end() && std::get<0>(it->second) != core_id) {
        // Address is in use by another core, pass the delay and results
        return {UINT32_MAX, std::get<1>(it->second), std::get<2>(it->second)};  // Return blocked access with delays
    }

    // Mark the address as in use by this core, initialize delays to 0
    addressInUse[address] = {core_id, 0, 0}; // Added third element for store and load delay

    // Perform the write operation
    auto result = ram.write(address, value, added_delay, bypass);

    // If bypass or operation completes, release the address
    if (bypass || result[0] == true) {
        addressInUse.erase(address);
    } else {
        // Update delays for ongoing operations
        addressInUse[address] = {core_id, result[1], result[2]};
    }

    return result;
}

// Read method to interact with RAM
std::vector<uint32_t> Membus::read(int core_id, uint32_t address, bool bypass) {
    // Check if the address is already in use by another core
    auto it = addressInUse.find(address);
    if (it != addressInUse.end() && std::get<0>(it->second) != core_id) {
        // Address is in use by another core, pass the delays
        return {UINT32_MAX-1, std::get<1>(it->second), std::get<2>(it->second)+1};  // Return blocked access with delays
    }

    // Mark the address as in use by this core, initialize delays to 0
    addressInUse[address] = {core_id, 0, 0}; // Added third element for store and load delay

    // Perform the read operation
    auto result = ram.read(address, bypass);

    // Update delays in `addressInUse` if the read has delays
    if (result[0] == UINT32_MAX) {
        addressInUse[address] = {core_id, result[1], result[2]};
    }

    // If bypass or operation completes, release the address
    if (bypass || result[0] != UINT32_MAX) {
        addressInUse.erase(address);
    }

    return result;
}

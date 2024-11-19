#include "membus.h"
#include <unordered_set>
#include <iostream>

// Constructor to initialize Membus with a reference to RAM
Membus::Membus(RAM& ramInstance) : ram(ramInstance) {}

// Adds a memory request to the queue
void Membus::addRequest(int cpuId, uint32_t address, bool write, uint32_t value) {
    // Check if the address is already in use
    if (addressInUse.find(address) == addressInUse.end()) {
        // Address is not in use, add it to the queue
        Request req = { cpuId, address, write, value };
        requestQueue.push(req);
        // Mark the address as in use
        addressInUse.insert(address);
    } else {
        // Address is already in use, add the request to the queue anyway
        std::cout << "Address 0x" << std::hex << address << " is already in use. Queuing request." << std::endl;
        Request req = { cpuId, address, write, value };
        requestQueue.push(req);
    }
}

// Process all the requests in the queue
void Membus::processRequests() {
    while (!requestQueue.empty()) {
        Request req = requestQueue.front();
        requestQueue.pop();

        if (req.write) {
            // Handle memory write: use the write method from RAM
            ram.write(req.address, req.data, 0, false);  // We pass 0 for added delay and false for bypass
        } else {
            // Handle memory read: use the read method from RAM
            ram.read(req.address, false);  // Pass false for bypass
        }

        // After processing, mark the address as no longer in use
        addressInUse.erase(req.address);
    }
}

// Write method to interact with RAM
std::vector<uint32_t> Membus::write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass) {
    // Call RAM's write method and return its result directly
    return ram.write(address, value, added_delay, bypass);
}

// Read method to interact with RAM
std::vector<uint32_t> Membus::read(uint32_t address, bool bypass) {
    // Adds the read request to the request queue
    addRequest(0, address, false, 0);  // Address is read, no value to set

    // Process the read request
    processRequests();  // Process the read request

    // Return the data from the last read operation (assuming you're using the RAM method)
    return ram.read(address, bypass);
}

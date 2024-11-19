#include "membus.h"

// Constructor to initialize Membus with a reference to RAM
Membus::Membus(RAM& ramInstance) : ram(ramInstance) {}

// Adds a memory request to the queue
void Membus::addRequest(int cpuId, uint32_t address, bool write, uint32_t value) {
    Request req = { cpuId, address, write, value };
    requestQueue.push(req);
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
    }
}

// Write method to interact with RAM
std::vector<uint32_t> Membus::write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass) {
    // Call RAM's write method and return its result directly
    return ram.write(address, value, added_delay, bypass);
}

// Read method to interact with RAM
std::vector<uint32_t> Membus::read(uint32_t address, bool bypass) {
    // Call RAM's read method and return its result directly
    return ram.read(address, bypass);
}

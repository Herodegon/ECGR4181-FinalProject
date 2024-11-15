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
            std::vector<uint32_t> data = ram.read(req.address, false);  // Pass false for bypass
            // You can now use the 'data' returned from RAM for any additional logic
        }
    }
}

// Write method to interact with RAM
uint32_t Membus::write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass) {
    // Handle memory write logic, and return status or delay in cycles
    if (!bypass) {
        // Immediate write (bypass any latency)
        ram.write(address, value, added_delay, bypass);
        return 0;  // 0 indicates no delay
    }

    // Simulate latency and return the number of cycles remaining
    return added_delay;  // Return the delay for the write operation
}

// Read method to interact with RAM
uint32_t Membus::read(uint32_t address, int cpuId) {
    // Adds the read request to the request queue
    addRequest(cpuId, address, false, 0);
    processRequests();  // Process the read request
    // After processing, we can return data from the last read operation (assuming you're using the RAM method)
    
    // Since RAM read returns a vector, you can either return a specific element or handle this differently
    std::vector<uint32_t> data = ram.read(address, false);  // We don't use bypass here

    // Return the first element of the vector (assuming you want a single 32-bit word)
    return data.empty() ? 0 : data[0];
}

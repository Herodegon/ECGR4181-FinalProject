#ifndef MEMBUS_H
#define MEMBUS_H

#include "ram.h"
#include <queue>
#include <cstdint>

class Membus {
public:
    struct Request {
        int cpuId;
        uint32_t address;
        bool write;
        uint32_t data;
    };

    // Constructor: Takes a reference to a RAM instance
    Membus(RAM& ramInstance);

    // Adds a memory request to the queue
    void addRequest(int cpuId, uint32_t address, bool write, uint32_t value);

    // Processes the requests in the queue
    void processRequests();

    // Write to memory and return the number of cycles remaining for completion
    uint32_t write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass);

    // Read from memory and return the data (or a specific value indicating a pending read)
    uint32_t read(uint32_t address, int cpuId);

private:
    RAM& ram;  // Reference to RAM object for memory operations
    std::queue<Request> requestQueue;  // Queue for memory requests
};

#endif // MEMBUS_H
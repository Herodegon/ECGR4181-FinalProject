// ram.cpp
#include "ram.h"

// Constructor: Initializes RAM and sets up specific memory regions
RAM::RAM() {
    std::memset(memory, 0, RAM_SIZE);   // Initialize RAM with zeroes
    initializeMemoryRegions();          // Initialize arrays with random FP32 values

    read_write_delay = 2;
}

// Read a 32-bit word from RAM with simulated latency
std::vector<uint32_t> RAM::read(uint32_t address, bool bypass) {
    if (address + 4 > RAM_SIZE) {
        throw std::out_of_range("RAM read out of bounds.");
    }

    std::vector<uint32_t> output;

    if (bypass){
        uint32_t value;
        std::memcpy(&value, &memory[address], sizeof(value));
        output = {value};
        return output; // Operation completed
    }

    AddressDelay& delays = addressDelays[address]; // Get or create delays for the address

    // First, handle the store delay
    if (delays.store > 0) {
        // Decrement store delay
        output = {UINT32_MAX, delays.store, delays.load};
        return output; // Operation pending
    }

    // Then, handle the load delay
    if (delays.load == 0) {
        // Set initial load delay
        delays.load = read_write_delay; // Subtract 1 because we'll decrement it now
        output = {UINT32_MAX, delays.store, delays.load};
        return output; // Operation pending
    } else if (delays.load > 1) {
        // Decrement load delay
        delays.load--;
        output = {UINT32_MAX, delays.store, delays.load};
        return output; // Operation pending
    } else if (delays.load == 1) {
        // Decrement load delay to zero and perform read
        delays.load = 0;
        uint32_t value;
        std::memcpy(&value, &memory[address], sizeof(value));
        output = {value, delays.store, delays.load};
        return output; // Operation completed
    }

    // Should not reach here
    return output;
}

// Write a 32-bit word to RAM with simulated latency
std::vector<uint32_t> RAM::write(uint32_t address, uint32_t value, uint32_t added_delay, bool bypass) {
    if (address + 4 > RAM_SIZE) {
        throw std::out_of_range("RAM write out of bounds.");
    }

    std::vector<uint32_t> output;

    if (bypass){
        std::memcpy(&memory[address], &value, sizeof(value));
        output = {true};
        return output; // Operation completed
    }

    AddressDelay& delays = addressDelays[address]; // Get or create delays for the address

    // Handle the store delay
    if (delays.store == 0) {
        // Set initial store delay
        delays.store = read_write_delay + added_delay; 
        output = {false, delays.store};
        return output; // Operation pending
    } else if (delays.store > 1) {
        // Decrement store delay
        delays.store--;
        output = {false, delays.store};
        return output; // Operation pending
    } else if (delays.store == 1) {
        // Decrement store delay to zero and perform write
        delays.store = 0;
        std::memcpy(&memory[address], &value, sizeof(value));
        output = {true, delays.store};
        return output; // Operation completed
    }

    // Should not reach here
    return output;
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
        write(address, value, 0, true);
        value += 1;
    }
    // for (uint32_t address = 0x400; address < 0x7FF; address += 4) {
    //     float random_value = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    //     uint32_t binary_value;
    //     std::memcpy(&binary_value, &random_value, sizeof(binary_value));
    //     write(address, binary_value, true);
    // }
    value = 1;
    for (uint32_t address = 0x800; address < 0xBFF; address += 4) {
        write(address, value, 0, true);
        value += 1;
    }
    // for (uint32_t address = 0x800; address < 0xBFF; address += 4) {
    //     float random_value = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    //     uint32_t binary_value;
    //     std::memcpy(&binary_value, &random_value, sizeof(binary_value));
    //     write(address, binary_value, true);
    // }
}

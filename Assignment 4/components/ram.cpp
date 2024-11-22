// ram.cpp
#include "ram.h"

// Constructor: Initializes RAM and sets up specific memory regions
RAM::RAM() {
    std::memset(memory, 0, RAM_SIZE);   // Initialize RAM with zeroes
    initializeMemoryRegions();          // Initialize arrays with random FP32 values
    initializeAddressDelays();
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
// Print memory contents for debugging
void RAM::print(uint32_t start, uint32_t end) const {
    for (uint32_t i = start; i < end; i += 4) {
        uint32_t intValue;
        std::memcpy(&intValue, &memory[i], sizeof(intValue));

        float floatValue;
        std::memcpy(&floatValue, &intValue, sizeof(floatValue));

        std::cout << floatValue << ' ';  // Print as a floating-point value
    }
    std::cout << '\n';
}

void RAM::printMath() {
    constexpr uint32_t arrayASize = 0x400; // Size of each array in bytes
    constexpr uint32_t elementSize = 4;   // Size of each element (FP32)

    // Iterate over the arrays and print calculations
    for (uint32_t i = 0; i < arrayASize / elementSize; ++i) {
        uint32_t addressA = 0x400 + i * elementSize;
        uint32_t addressB = 0x800 + i * elementSize;
        uint32_t addressC = 0xC00 + i * elementSize;
        uint32_t addressD = 0x1000 + i * elementSize;

        float valueA, valueB, valueC, valueD;

        // Read values from RAM
        std::memcpy(&valueA, &memory[addressA], sizeof(valueA));
        std::memcpy(&valueB, &memory[addressB], sizeof(valueB));
        std::memcpy(&valueC, &memory[addressC], sizeof(valueC));
        std::memcpy(&valueD, &memory[addressD], sizeof(valueD));

        // Print addition
        std::cout << "Array_A[" << i << "] + Array_B[" << i << "] = Array_C[" << i << "] | "
                  << valueA << " + " << valueB << " = " << valueC << std::endl;

        // Print subtraction
        std::cout << "Array_A[" << i << "] - Array_B[" << i << "] = Array_D[" << i << "] | "
                  << valueA << " - " << valueB << " = " << valueD << std::endl;
    }
}

// Initialize specific memory regions as per specifications
void RAM::initializeMemoryRegions() {
    // Seed the random number generator for reproducibility
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Helper lambda to generate constrained random FP32 values
    auto generateRandomFP32 = []() -> float {
        float randomValue;
        uint32_t intValue;

        do {
            randomValue = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // Scale to [0.0, 1.0]
            std::memcpy(&intValue, &randomValue, sizeof(intValue)); // Convert FP32 to uint32_t
        } while (intValue == UINT32_MAX || intValue == UINT32_MAX - 1); // Regenerate if prohibited values

        return randomValue;
    };

    // Initialize ARRAY_A (0x400 - 0x7FF) with random FP32 values in [0.0, 1.0]
    for (uint32_t address = 0x400; address < 0x800; address += 4) {
        float randomValue = generateRandomFP32();
        uint32_t value;
        std::memcpy(&value, &randomValue, sizeof(value)); // Convert FP32 to uint32_t
        write(address, value, 0, true);
    }

    // Initialize ARRAY_B (0x800 - 0xBFF) with random FP32 values in [0.0, 1.0]
    for (uint32_t address = 0x800; address < 0xC00; address += 4) {
        float randomValue = generateRandomFP32();
        uint32_t value;
        std::memcpy(&value, &randomValue, sizeof(value)); // Convert FP32 to uint32_t
        write(address, value, 0, true);
    }
}

// Initialize addressDelays for all addresses to zero
void RAM::initializeAddressDelays() {
    for (uint32_t address = 0; address < RAM_SIZE; address += 4) {
        AddressDelay& delays = addressDelays[address];
        delays.store = 0;
        delays.load = 0;
    }
    std::cout << "AddressDelays initialized for all addresses." << std::endl;
}

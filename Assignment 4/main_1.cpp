#include "components/core.h"
#include "components/simulator.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./core <program0.bin> [<program1.bin>]" << std::endl;
        return 1;
    }

    std::string instruction_file_0 = argv[1]; 
    std::string instruction_file_1;
    uint32_t instruction_address_0 = 0x0000;
    uint32_t instruction_address_1 = 0x0200;

    int limit = 25000;

    // Create the simulator with the specified limit
    Simulator sim(limit);

    // Create core0 and add it to the simulator
    Core* core0 = new Core(instruction_address_0, 0, 0x2FF);
    sim.add_core(core0);

    // Load instructions for core0 into RAM using Membus
    sim.load_instructions_from_binary(core0, instruction_file_0, instruction_address_0);

    // If a second program is provided, create core1 and load instructions
    if (argc >= 3) {
        instruction_file_1 = argv[2];

        Core* core1 = new Core(instruction_address_1, 1, 0x3FF);
        sim.add_core(core1);
        sim.load_instructions_from_binary(core1, instruction_file_1, instruction_address_1);
    }

    // Run the simulation
    sim.run();

    sim.get_ram()->printMath();

    // Print results
    // For debugging purposes, we can access RAM directly
    std::cout << "ARRAY A: " << std::endl << "--------------------------------------------------" << std::endl;
    sim.get_ram()->print(0x400, 0x7FF); // 0x7FF
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "ARRAY B: " << std::endl << "--------------------------------------------------" << std::endl;
    sim.get_ram()->print(0x800, 0xBFF); // 0xBFF
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "ARRAY C: " << std::endl << "--------------------------------------------------" << std::endl;
    sim.get_ram()->print(0xC00, 0xFFF); // 0xFFF
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "ARRAY D: " << std::endl << "--------------------------------------------------" << std::endl;
    sim.get_ram()->print(0x1000, 0x13FF); // 0x13FF

    return 0;
}

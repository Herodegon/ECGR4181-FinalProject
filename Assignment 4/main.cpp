#include "components/simulator.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./simulator <program.bin>" << std::endl;
        return 1;
    }

    std::string filename = argv[1]; // Get the filename from command-line arguments
    int limit = 1000;
    Simulator sim(limit);
    sim.load_instructions_from_binary(filename);
    sim.run();
    std::cout << "ARRAY A: ";
    sim.ram.print(0x400, 0x7FF);
    std::cout << "ARRAY B: ";
    sim.ram.print(0x800, 0xBFF);
    std::cout << "ARRAY C: ";
    sim.ram.print(0xC00, 0xFFF);
    return 0;
}
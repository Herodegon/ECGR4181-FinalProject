#include "components/simulator.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./simulator <program.bin>" << std::endl;
        return 1;
    }

    std::string filename = argv[1]; // Get the filename from command-line arguments
    int limit = 10;
    Simulator sim(limit);
    sim.load_instructions_from_binary(filename);
    sim.run();
    return 0;
}
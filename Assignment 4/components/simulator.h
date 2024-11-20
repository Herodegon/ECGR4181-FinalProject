#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include <string>
#include "core.h"
#include "ram.h"
#include "membus.h"

class Simulator {
private:
    std::vector<Core*> cores;
    RAM ram;
    Membus membus;
    int clock_cycle_limit;

public:
    Simulator(int num_runs = 0);
    void add_core(Core* core);
    void load_instructions_from_binary(Core* core, const std::string& filename, uint32_t start_address);
    void run();
    RAM* get_ram();
    Membus* get_membus();
};

#endif // SIMULATOR_H

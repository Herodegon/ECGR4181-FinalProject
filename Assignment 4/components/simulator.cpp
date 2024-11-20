#include "simulator.h"
#include <iostream>
#include <fstream>

Simulator::Simulator(int num_runs)
    : clock_cycle_limit(num_runs), membus(ram) {
}

void Simulator::add_core(Core* core) {
    core->set_membus(&membus);
    cores.push_back(core);
}

void Simulator::load_instructions_from_binary(Core* core, const std::string& filename, uint32_t start_address) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open binary file: " + filename);
    }

    uint32_t address = start_address;

    while (infile.peek() != std::ifstream::traits_type::eof()) {
        uint32_t instruction;
        infile.read(reinterpret_cast<char*>(&instruction), sizeof(instruction));

        if (!infile) {
            if (infile.eof()) break;
            throw std::runtime_error("Error reading from binary file: " + filename);
        }

        membus.write(core->core_id, address, instruction, 0, true); // ram.write(address, instruction, 0, true);
        address += 4;
    }
    core->start_address = start_address;
    core->max_instruction_address = address - 4;

    infile.close();
}

RAM* Simulator::get_ram() {
    return &ram;
}

Membus* Simulator::get_membus() {
    return &membus;
}

void Simulator::run() {
    int clock_cycle = 0;
    std::map<Core*, int> core_instruction_counts; // To track instruction counts for each core
    std::map<Core*, int> core_clock_cycles;      // To track clock cycles for each core

    // Initialize instruction counts and clock cycles for each core
    for (auto core : cores) {
        core_instruction_counts[core] = 0; // Assume you have a way to get initial instruction counts
        core_clock_cycles[core] = 0;
    }

    while (true) {
        clock_cycle++;

        std::cout << "Cycle " << clock_cycle << "\n";
        // std::cout << "--------------------------------------------------" << std::endl;

        bool all_cores_completed = true;
        for (auto core : cores) {
            if (!core->is_complete()) {
                std::cout << "--------------------------------------------------" << std::endl;
                std::cout << "CORE " << core->core_id << std::endl;
                std::cout << "--------------------------------------------------" << std::endl;
                core->store();
                core->execute();
                core->decode();
                core->fetch();
                all_cores_completed = false;
                core_clock_cycles[core]++; // Increment clock cycle count for the core
            }
        }

        std::cout << "--------------------------------------------------" << std::endl;

        if (clock_cycle_limit != 0 && clock_cycle >= clock_cycle_limit) break;

        if (all_cores_completed) {
            std::cout << "Simulation completed at clock cycle: " << clock_cycle << std::endl;

            // Calculate and print CPI for each core
            for (auto core : cores) {
                int instructions = core->instruction_count;
                int cycles = core_clock_cycles[core];
                double cpi = instructions > 0 ? static_cast<double>(cycles) / instructions : 0.0;

                std::cout << "Core " << core->core_id << " completed at clock cycle: " << cycles << std::endl;
                std::cout << "Instruction count: " << instructions << std::endl;
                std::cout << "Average CPI: " << cpi << std::endl;
            }
            break;
        }
    }
}

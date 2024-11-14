// simulator.h

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <bitset>
#include <limits>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "decoder.h"
#include "ram.h"

const int STALL_INT = 10;       // Stall for integer instructions = 1 CPU cycle = 10 sim ticks
const int STALL_FLOAT = 50;     // Stall for floating point instructions = 5 CPU cycles = 50 sim ticks

struct Event {
    std::string name;
    std::string stage;
    int cycle;
};

struct Instruction {
    std::string name;
    uint32_t binary;
    std::vector<std::string> operands;
    std::string type;
    std::string stage;
    double data;
    std::map<std::string, int> cycle_entered;
    Instruction(std::string n, uint32_t b, std::vector<std::string> ops, std::string t)
        : name(n), binary(b), operands(ops), type(t), stage("Fetch"), data(0.0) {}
};

const std::vector<std::string> pipeline_stages = {"Fetch", "Decode", "Execute", "Store"};

class Simulator {
private:
    uint32_t max_instruction_address;
    int clock_cycle;
    int store_counter;
    int excecute_counter;
    int decode_counter;
    int fetch_delay = 0;
    int decode_delay = 0;
    int execute_delay = 0;
    int store_delay = 0;
    const int clock_cycle_limit;
    int sim_ticks;
    int pc;
    std::vector<Event> event_list;
    std::map<std::string, Instruction*> pipeline_registers;
    std::vector<Instruction*> instructions;
    std::map<std::string, int> registers;
    std::map<std::string, double> f_registers;
    bool halt;
    int stall_count;
    Decoder decoder;

public:
    RAM ram;
    Simulator(int num_runs = 0);
    int delay = 0;
    void load_instructions_from_binary(const std::string& filename);
    void fetch();
    void decode();
    void execute();
    void store();
    void clean_event_list(Instruction* instr);
    void execute_instruction(Instruction* instr);
    void delay_cycles(int cycle_count);
    std::string to_hex_string(uint32_t instruction);
    std::vector<std::string> split_instruction(const std::string& instruction);
    void flush_pipeline();
    void print_event_list();
    void print_instructions();
    void print_pipeline_registers();
    void print_registers();
    void print_f_registers();
    void run();
};

#endif // SIMULATOR_H

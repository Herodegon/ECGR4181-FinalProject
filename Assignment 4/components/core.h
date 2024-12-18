#ifndef CORE_H
#define CORE_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <climits>
#include <bitset>
#include <limits>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "decoder.h"
#include "membus.h"
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
    int execute_delay;
    int store_delay;
    double data;
    std::map<std::string, int> cycle_entered;
    Instruction(std::string n, uint32_t b, std::vector<std::string> ops, std::string t)
        : name(n), binary(b), operands(ops), type(t), stage("Fetch"), execute_delay(0), store_delay(0), data(0.0) {}
};

const std::vector<std::string> pipeline_stages = {"Fetch", "Decode", "Execute", "Store"};

class Core {
private:
    int clock_cycle;
    int store_counter;
    int excecute_counter;
    int decode_counter;
    int fetch_delay = 0;
    int decode_delay = 0;
    int execute_delay = 0;
    int store_delay = 0;
    int fetching_active = 1;
    int complete = 0;
    bool store_complete = false;
    bool execute_delay_complete = false;
    bool store_delay_complete = false;
    int sim_ticks;
    
    std::vector<Event> event_list;
    std::map<std::string, Instruction*> pipeline_registers;
    std::vector<Instruction*> instructions;
    std::map<std::string, int> registers;
    std::map<std::string, bool> hold_registers;
    bool halt;
    int stall_count;
    Decoder decoder;
    RAM* ram; 
    Membus* membus;

public:
    Core(int start_pc, int core_id, uint32_t initial_sp);
    int core_id; 
    int pc;
    uint32_t max_instruction_address;
    uint32_t start_address;
    int instruction_count = 0;
    int delay = 0;
    void set_ram(RAM* ram_ptr);
    void set_membus(Membus* membus_ptr);
    void fetch();
    void decode();
    void execute();
    void store();
    void clean_event_list(Instruction* instr);
    void execute_instruction(Instruction*, std::string, std::vector<std::string>);
    void store_instruction(std::string, std::vector<std::string>, int);
    int delay_cycles(int cycle_count);
    std::string to_hex_string(uint32_t instruction);
    std::vector<std::string> split_instruction(const std::string& instruction);
    void flush_pipeline();
    void print_event_list();
    void print_instructions();
    void print_pipeline_registers();
    void print_registers();
    void print_f_registers();
    bool is_halted() const;
    bool is_complete() const;
};

#endif // CORE_H

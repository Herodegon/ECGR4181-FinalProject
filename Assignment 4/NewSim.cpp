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

//====ASSIGNMENT 4 Part 1====

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
    Instruction( std::string n, uint32_t b, std::vector<std::string> ops, std::string t) : name(n), binary(b), operands(ops), type(t), stage("Fetch"), data(0.0) {}
};

const std::vector<std::string> pipeline_stages = {"Fetch", "Decode", "Execute", "Store"};

class Simulator {
private:
    int clock_cycle;
    const int clock_cycle_limit;
    int sim_ticks;
    int pc;                                                     // Program counter
    std::vector<Event> event_list;
    std::map<std::string, Instruction*> pipeline_registers;
    std::vector<Instruction*> instructions;
    std::map<std::string, int> registers;
    std::map<std::string, double> f_registers;
    std::map<int, double> memory;
    bool halt;
    int stall_count;

public:
    Decoder decoder;
    Simulator(int num_runs = 0) 
        : clock_cycle(0), sim_ticks(0), clock_cycle_limit(num_runs), pc(0), halt(false), stall_count(0) {
        pipeline_registers["Fetch"] = nullptr;
        pipeline_registers["Decode"] = nullptr;
        pipeline_registers["Execute"] = nullptr;
        pipeline_registers["Store"] = nullptr;
        // Initialize registers and memory
        registers["x10"] = 0; // Base address for ARRAY_A
        registers["x11"] = 0; // Base address for ARRAY_B
        registers["x12"] = 0; // Base address for ARRAY_C
        registers["x13"] = 0; // Loop counter i
    }

   void load_instructions_from_binary(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open binary file: " + filename);
    }

    // Read instructions from the binary file and store them directly.
    while (infile.peek() != std::ifstream::traits_type::eof()) {
        uint32_t instruction;
        infile.read(reinterpret_cast<char*>(&instruction), sizeof(instruction));

        if (!infile) {
            if (infile.eof()) {
                std::cerr << "Reached end of file while reading." << std::endl;
                break;
            }
            throw std::runtime_error("Error reading from binary file: " + filename);
        }

        // Convert instruction to hex string and store it in the instructions vector.
        std::string hex_value = to_hex_string(instruction);
        instructions.push_back(new Instruction(hex_value, instruction, {}, "Binary"));
    }

    infile.close();
}

    void fetch() {
        if (stall_count > 0) {
            std::cout << "Cycle " << clock_cycle << ": Fetch stage is stalled." << std::endl;
            stall_count--;
            return;
        }

        if (pc < instructions.size()) {
            Instruction* instr = instructions[pc];
            pc++;
            instr->stage = "Fetch";
            instr->cycle_entered["Fetch"] = clock_cycle;
            pipeline_registers["Fetch"] = instr;
            event_list.push_back({instr->name, "Fetch", clock_cycle});
            std::cout << "Cycle " << clock_cycle << ": Fetching instruction " << instr->name << std::endl;
        } else {
            pipeline_registers["Fetch"] = nullptr; // If no more instructions, set fetch stage to null
            halt = true; // No more instructions to execute
        }
    }

    std::string to_hex_string(uint32_t instruction) {
        std::ostringstream oss;
        oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << instruction;
        return oss.str();
    }

    // In Sim.cpp
    void decode() {
        if (pipeline_registers["Fetch"] != nullptr) {
            Instruction* fetched_instr = pipeline_registers["Fetch"];
            uint32_t instruction_value = fetched_instr->binary;
            
            // Capture the decoded instruction name
            std::string decodedName = decoder.decodeInstruction(instruction_value);
            
            // Split the decoded instruction into the operation and operands
            std::vector<std::string> operands = split_instruction(decodedName);
            
            // Update operands in the Instruction object
            fetched_instr->operands = operands;
            
            std::cout << "Operands: ";
            for (const auto& operand : operands) {
                std::cout << operand << " ";
            }
            std::cout << std::endl;

            pipeline_registers["Decode"] = fetched_instr;
            pipeline_registers["Fetch"] = nullptr;
        } else {
            std::cout << "Cycle " << clock_cycle << ": No instruction to decode." << std::endl;
        }
    }

    // Helper function to split the decoded instruction into operands
    std::vector<std::string> split_instruction(const std::string& instruction) {
    std::vector<std::string> operands;
    std::istringstream iss(instruction);
    std::string token;

    // Split the instruction by the first space (this should capture the operation)
    if (std::getline(iss, token, ' ')) {
        operands.push_back(token); // Store the operation (e.g., "addi")
    }

    // Now handle the rest of the instruction (which are operands)
    std::string remaining;
    if (std::getline(iss, remaining)) {
        std::istringstream operand_stream(remaining);
        while (std::getline(operand_stream, token, ',')) {
            // Remove leading and trailing spaces
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            operands.push_back(token); // Store each operand
        }
    }

    return operands;
}


    void execute() {
        if (stall_count > 0) {
            std::cout << "Cycle " << clock_cycle << ": Execute stage is stalled." << std::endl;
            stall_count--;
            return;
        }

        Instruction* instr = pipeline_registers["Decode"];
        if (instr) {
            instr->stage = "Execute";
            instr->cycle_entered["Execute"] = clock_cycle;
            execute_instruction(instr);
            event_list.push_back({instr->name, "Execute", clock_cycle});
            std::cout << "Cycle " << clock_cycle << ": Executing instruction " << instr->name << std::endl;
            pipeline_registers["Execute"] = instr;
            pipeline_registers["Decode"] = nullptr; // Clear the Decode stage
        } else {
            pipeline_registers["Execute"] = nullptr;   
            std::cout << "Cycle " << clock_cycle << ": No instruction to execute." << std::endl;
        }
    }

    void store() {
        Instruction* instr = pipeline_registers["Execute"];
        if (instr) {
            instr->stage = "Store";
            instr->cycle_entered["Store"] = clock_cycle;
            event_list.push_back({instr->name, "Store", clock_cycle});
            std::cout << "Cycle " << clock_cycle << ": Storing results for instruction " << instr->name << std::endl;
            pipeline_registers["Store"] = instr;
            pipeline_registers["Execute"] = nullptr; // Clear the Execute stage
        } else {
            std::cout << "Cycle " << clock_cycle << ": No instruction to store." << std::endl;
        }
    }

    void clean_event_list(Instruction* instr) {
        for (auto it = event_list.begin(); it != event_list.end();) {
            if (it->name == instr->name && it->stage == instr->stage) {
                it = event_list.erase(it);
            } else {
                ++it;
            }
        }
    }

    void execute_instruction(Instruction* instr) {
        if (!instr) {
            std::cout << "No instruction to execute." << std::endl;
            return;
        }

        std::vector<std::string>& operands = instr->operands;  // Get operands from instruction

        // Ensure the instruction has at least one operand (the name)
        if (operands.empty()) {
            std::cout << "Invalid instruction, no operands provided." << std::endl;
            return;
        }

        std::string name = operands[0];  // The first operand is the instruction name

        if (name == "la") {
            // Load address into the register
            if (operands.size() >= 2) {
                if (operands[1] == "x10") {
                    registers["x10"] = 0x0400; // Address for ARRAY_A
                    std::cout << "Loaded address 0x0400 into x10." << std::endl;
                } else if (operands[1] == "x11") {
                    registers["x11"] = 0x0800; // Address for ARRAY_B
                    std::cout << "Loaded address 0x0800 into x11." << std::endl;
                } else if (operands[1] == "x12") {
                    registers["x12"] = 0x0C00; // Address for ARRAY_C
                    std::cout << "Loaded address 0x0C00 into x12." << std::endl;
                }
            }
        } else if (name == "li") {
            // Load immediate value into the register
            if (operands.size() >= 3) {
                registers[operands[1]] = std::stoi(operands[2]);
                std::cout << "Loaded immediate " << operands[2] << " into " << operands[1] << "." << std::endl;
            }
        } else if (name == "flw") {
            // Load floating point word from memory into floating-point register
            if (operands.size() >= 3) {
                std::string f_reg = operands[1]; // e.g., "f0"
                std::string addr_reg = operands[2]; // e.g., "0(x10)"
                
                // Extract base register (assuming format like "0(x10)")
                int base_addr = registers[addr_reg.substr(3, 3)]; // Extract register (e.g., x10)
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('('))); // Extract offset (e.g., 0)

                double value = memory[base_addr + offset]; // Load value from calculated address
                f_registers[f_reg] = value; // Store in floating-point register
                std::cout << "Loaded " << value << " into " << f_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "fadd.s") {
            // Floating point addition
            if (operands.size() >= 4) {
                std::string dest_reg = operands[1]; // Destination register
                std::string src_reg1 = operands[2]; // First source register
                std::string src_reg2 = operands[3]; // Second source register

                f_registers[dest_reg] = f_registers[src_reg1] + f_registers[src_reg2];
                std::cout << "Added " << f_registers[src_reg1] << " and " << f_registers[src_reg2] 
                        << ", result in " << dest_reg << ": " << f_registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "fsw") {
            // Store floating point word from register into memory
            if (operands.size() >= 3) {
                std::string f_reg = operands[1]; // e.g., "f2"
                std::string addr_reg = operands[2]; // e.g., "0(x12)"

                int base_addr = registers[addr_reg.substr(3, 3)];
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('(')));

                memory[base_addr + offset] = f_registers[f_reg];
                std::cout << "Stored " << f_registers[f_reg] << " from " << f_reg 
                        << " into memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "auipc") {
            // Add Upper Immediate to PC
            if (operands.size() >= 3) {
                std::string reg = operands[1];
                int immediate = std::stoi(operands[2], nullptr, 16); // Convert hex string to integer
                registers[reg] = pc + (immediate << 12); // Shift left by 12 bits
                std::cout << "AUIPC: Loaded " << registers[reg] << " into " << reg << " with immediate " << immediate << "." << std::endl;
            }
        } else if (name == "addi") {
            // Add Immediate
            if (operands.size() >= 4) {
                std::string dest_reg = operands[1];
                std::string src_reg = operands[2];
                int immediate = std::stoi(operands[3]);

                registers[dest_reg] = registers[src_reg] + immediate;
                std::cout << "ADDI: Added " << immediate << " to " << src_reg << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "jalr") {
            // Jump and Link Register
            if (operands.size() >= 3) {
                std::string dest_reg = operands[1];
                int offset = std::stoi(operands[2]);

                pc += offset; // Jump to the address
                registers[dest_reg] = pc; // Save return address
                std::cout << "JALR: Jumped to address " << pc << ", return address in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "slli") {
            // Shift Left Logical Immediate
            if (operands.size() >= 4) {
                std::string dest_reg = operands[1];
                std::string src_reg = operands[2];
                int shift_amount = std::stoi(operands[3]);

                registers[dest_reg] = registers[src_reg] << shift_amount;
                std::cout << "SLLI: Shifted " << src_reg << " left by " << shift_amount << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "lw") {
            // Load Word
            if (operands.size() >= 3) {
                std::string dest_reg = operands[1];
                std::string addr_reg = operands[2];

                int base_addr = registers[addr_reg.substr(3, 3)]; // Extract base register
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('('))); // Extract offset

                registers[dest_reg] = memory[base_addr + offset]; // Load value from memory
                std::cout << "LW: Loaded " << registers[dest_reg] << " into " << dest_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "sw") {
            // Store Word
            if (operands.size() >= 3) {
                std::string src_reg = operands[1];
                std::string addr_reg = operands[2];

                int base_addr = registers[addr_reg.substr(3, 3)];
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('(')));

                memory[base_addr + offset] = registers[src_reg]; // Store value to memory
                std::cout << "SW: Stored " << registers[src_reg] << " into memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "beq") {
            // Branch if Equal
            if (operands.size() >= 4) {
                std::string reg1 = operands[1];
                std::string reg2 = operands[2];
                int offset = std::stoi(operands[3]);

                if (registers[reg1] == registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BEQ: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BEQ: No branch taken." << std::endl;
                }
            }
        } else if (name == "bne") {
            // Branch if Not Equal
            if (operands.size() >= 4) {
                std::string reg1 = operands[1];
                std::string reg2 = operands[2];
                int offset = std::stoi(operands[3]);

                if (registers[reg1] != registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BNE: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BNE: No branch taken." << std::endl;
                }
            }
        } else {
            std::cout << "Unsupported instruction: " << name << std::endl;
        }
    }

    void flush_pipeline()
        {
            for (auto& stage : pipeline_stages)
            {
                if (stage != "Store")
                {
                    pipeline_registers[stage] = nullptr;
                }
            }
        }
    void print_event_list()
        {
            std::cout << '\n'
                      << "Event List at Cycle " << clock_cycle << ":" << std::endl;
            for (auto& event : event_list)
            {
                std::cout << "Instruction " << event.name << " in " << event.stage << " stage at cycle " << event.cycle << std::endl;
            }
        }

    void print_instructions()
        {
            std::cout << '\n'
                      << "Instructions:" << std::endl;
            for (auto& instr : instructions)
            {
                std::cout << instr->name << " ";
                for (auto& op : instr->operands)
                {
                    std::cout << op << " ";
                }
                std::cout << std::endl;
            }
        }
    void print_pipeline_registers()
        {
            std::cout << '\n'
                      << "Pipeline Registers:" << std::endl;
            for (auto& reg : pipeline_registers)
            {
                std::cout << reg.first << ": " << reg.second << std::endl;
            }
        }

    void print_registers()
        {
            std::cout << '\n'
                      << "Registers:" << std::endl;
            for (auto& reg : registers)
            {
                std::cout << reg.first << ": " << reg.second << std::endl;
            }
        }

    void print_f_registers()
        {
            std::cout << '\n'
                      << "Floating Point Registers:" << std::endl;
            for (auto& reg : f_registers)
            {
                std::cout << reg.first << ": " << reg.second << std::endl;
            }
        }

            void run() 
        {
            while (!halt || pipeline_registers["Fetch"] || pipeline_registers["Decode"] || pipeline_registers["Execute"] || pipeline_registers["Store"]) {
                sim_ticks++;
                if(sim_ticks % 10 == 0) {
                    clock_cycle++;
                    sim_ticks = 0;
                }
                std::cout << "--------------------------------------------------" << std::endl;
                store();
                execute();
                decode();
                fetch();

                if (stall_count > 0)
                {
                    stall_count--;
                }

                print_event_list();
                print_instructions();
                //print_pipeline_registers();
                //print_f_registers();
                print_registers();
                if (clock_cycle_limit != 0 && clock_cycle >= clock_cycle_limit)
                {
                    break;
                }
                if (halt)
                {
                    flush_pipeline();
                    break;
                }
            }
        }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./simulator <program.bin>" << std::endl;
        return 1;
    }

    std::string filename = argv[1]; // Get the filename from command-line arguments
    int limit = 100;
    Simulator sim(limit);
    sim.load_instructions_from_binary(filename);
    sim.run();
    return 0;
}

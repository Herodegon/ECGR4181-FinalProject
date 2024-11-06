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
    std::vector<std::string> operands;
    std::string type;
    std::string stage;
    double data;
    std::map<std::string, int> cycle_entered;
    Instruction(std::string n, std::vector<std::string> ops, std::string t) : name(n), operands(ops), type(t), stage("Fetch"), data(0.0) {}
};

const std::vector<std::string> pipeline_stages = {"Fetch", "Decode", "Execute", "Store"};

class Simulator {
private:
    int clock_cycle;
    const int clock_cycle_limit;
    int pc;                                                     // Program counter
    std::vector<Event> event_list;
    std::map<std::string, Instruction*> pipeline_registers;
    std::vector<Instruction*> instructions;
    std::map<std::string, int> registers;
    std::map<std::string, double> f_registers;
    std::map<int, double> memory;
    bool halt;
    int stall_count;
    std::unordered_map<uint32_t, std::string> instruction_map = {
        {0x00000100, "UNKNOWN_0x00000100"},
        {0x00000400, "LOAD_BASE_A"},
        {0x00000517, "auipc a0, 0x0"},
        {0x00000597, "auipc a1, 0x0"},
        {0x00000617, "auipc a2, 0x0"},
        {0x00000800, "LOAD_BASE_B"},
        {0x00000C00, "LOAD_BASE_C"},
        {0x00050513, "addi a0, a0, 5"},
        {0x00058593, "addi a1, a1, 5"},
        {0x00060613, "addi a2, a2, 6"},
        {0x05110026, "jalr t0, 0x51"},
        {0x06110591, "slli t1, t1, 1"},
        {0x07134681, "lw t0, 0(t1)"},
        {0x200700E6, "beq a4, a0, 0xE6"},
        {0x20270010, "bne a4, a0, 0x10"},
        {0x71530005, "sw a1, 5(t1)"},
        {0xA0870005, "lw a5, 5(a0)"},
        {0xB7C50685, "jal t0, 0x6"},
        {0xDF631000, "ecall"}
    };

public:
    Simulator(int num_runs = 0) 
        : clock_cycle(0), clock_cycle_limit(num_runs), pc(0), halt(false), stall_count(0) {
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

        std::set<std::string> instruction_set;

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

            // Convert instruction to hex string
            std::string hex_value = to_hex_string(instruction);
            if (instruction_set.insert(hex_value).second) {
                instructions.push_back(new Instruction(hex_value, {}, "Binary"));
            }
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

    void decode() {
        if (pipeline_registers["Fetch"] != nullptr) {
            Instruction* fetched_instr = pipeline_registers["Fetch"];
            uint32_t instruction_value = std::stoul(fetched_instr->name, nullptr, 0);

            auto it = instruction_map.find(instruction_value);
            if (it != instruction_map.end()) {
                std::cout << "Cycle " << clock_cycle << ": Decoding instruction: " << it->second << std::endl;
                execute_instruction(fetched_instr);
            } else {
                std::cout << "Cycle " << clock_cycle << ": Instruction " << fetched_instr->name << " is not recognized." << std::endl;
            }

            pipeline_registers["Decode"] = fetched_instr;
            pipeline_registers["Fetch"] = nullptr;
        } else {
            std::cout << "Cycle " << clock_cycle << ": No instruction to decode." << std::endl;
        }
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

        std::string name = instr->name;

        if (name == "la") {
            // Load address into the register (hardcoded addresses for simplicity)
            if (instr->operands[0] == "x10") {
                registers["x10"] = 0x0400; // Address for ARRAY_A
                std::cout << "Loaded address 0x0400 into x10." << std::endl;
            } else if (instr->operands[0] == "x11") {
                registers["x11"] = 0x0800; // Address for ARRAY_B
                std::cout << "Loaded address 0x0800 into x11." << std::endl;
            } else if (instr->operands[0] == "x12") {
                registers["x12"] = 0x0C00; // Address for ARRAY_C
                std::cout << "Loaded address 0x0C00 into x12." << std::endl;
            }
        } else if (name == "li") {
            // Load immediate value into the register
            if (instr->operands.size() >= 2) {
                registers[instr->operands[0]] = std::stoi(instr->operands[1]);
                std::cout << "Loaded immediate " << instr->operands[1] << " into " << instr->operands[0] << "." << std::endl;
            }
        } else if (name == "flw") {
            // Load floating point word from memory into floating-point register
            if (instr->operands.size() >= 2) {
                std::string f_reg = instr->operands[0]; // e.g., "f0"
                std::string addr_reg = instr->operands[1]; // e.g., "0(x10)"
                
                // Extract base register (assuming format like "0(x10)")
                int base_addr = registers[addr_reg.substr(3, 3)]; // Extract register (e.g., x10)
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('('))); // Extract offset (e.g., 0)

                double value = memory[base_addr + offset]; // Load value from calculated address
                f_registers[f_reg] = value; // Store in floating-point register
                std::cout << "Loaded " << value << " into " << f_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "fadd.s") {
            // Floating point addition
            if (instr->operands.size() >= 3) {
                std::string dest_reg = instr->operands[0]; // Destination register
                std::string src_reg1 = instr->operands[1]; // First source register
                std::string src_reg2 = instr->operands[2]; // Second source register

                f_registers[dest_reg] = f_registers[src_reg1] + f_registers[src_reg2];
                std::cout << "Added " << f_registers[src_reg1] << " and " << f_registers[src_reg2] 
                        << ", result in " << dest_reg << ": " << f_registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "fsw") {
            // Store floating point word from register into memory
            if (instr->operands.size() >= 2) {
                std::string f_reg = instr->operands[0]; // e.g., "f2"
                std::string addr_reg = instr->operands[1]; // e.g., "0(x12)"

                int base_addr = registers[addr_reg.substr(3, 3)];
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('(')));

                memory[base_addr + offset] = f_registers[f_reg];
                std::cout << "Stored " << f_registers[f_reg] << " from " << f_reg 
                        << " into memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "auipc") {
            // Add Upper Immediate to PC
            if (instr->operands.size() >= 2) {
                std::string reg = instr->operands[0];
                int immediate = std::stoi(instr->operands[1], nullptr, 16); // Convert hex string to integer
                registers[reg] = pc + (immediate << 12); // Shift left by 12 bits
                std::cout << "AUIPC: Loaded " << registers[reg] << " into " << reg << " with immediate " << immediate << "." << std::endl;
            }
        } else if (name == "addi") {
            // Add Immediate
            if (instr->operands.size() >= 3) {
                std::string dest_reg = instr->operands[0];
                std::string src_reg = instr->operands[1];
                int immediate = std::stoi(instr->operands[2]);

                registers[dest_reg] = registers[src_reg] + immediate;
                std::cout << "ADDI: Added " << immediate << " to " << src_reg << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "jalr") {
            // Jump and Link Register
            if (instr->operands.size() >= 2) {
                std::string dest_reg = instr->operands[0];
                int offset = std::stoi(instr->operands[1]);

                pc += offset; // Jump to the address
                registers[dest_reg] = pc; // Save return address
                std::cout << "JALR: Jumped to address " << pc << ", return address in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "slli") {
            // Shift Left Logical Immediate
            if (instr->operands.size() >= 3) {
                std::string dest_reg = instr->operands[0];
                std::string src_reg = instr->operands[1];
                int shift_amount = std::stoi(instr->operands[2]);

                registers[dest_reg] = registers[src_reg] << shift_amount;
                std::cout << "SLLI: Shifted " << src_reg << " left by " << shift_amount << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
            }
        } else if (name == "lw") {
            // Load Word
            if (instr->operands.size() >= 2) {
                std::string dest_reg = instr->operands[0];
                std::string addr_reg = instr->operands[1];

                int base_addr = registers[addr_reg.substr(3, 3)]; // Extract base register
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('('))); // Extract offset

                registers[dest_reg] = memory[base_addr + offset]; // Load value from memory
                std::cout << "LW: Loaded " << registers[dest_reg] << " into " << dest_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "sw") {
            // Store Word
            if (instr->operands.size() >= 2) {
                std::string src_reg = instr->operands[0];
                std::string addr_reg = instr->operands[1];

                int base_addr = registers[addr_reg.substr(3, 3)];
                int offset = std::stoi(addr_reg.substr(0, addr_reg.find('(')));

                memory[base_addr + offset] = registers[src_reg]; // Store value to memory
                std::cout << "SW: Stored " << registers[src_reg] << " into memory address " << (base_addr + offset) << "." << std::endl;
            }
        } else if (name == "beq") {
            // Branch if Equal
            if (instr->operands.size() >= 3) {
                std::string reg1 = instr->operands[0];
                std::string reg2 = instr->operands[1];
                int offset = std::stoi(instr->operands[2]);

                if (registers[reg1] == registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BEQ: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BEQ: No branch taken." << std::endl;
                }
            }
        } else if (name == "bne") {
            // Branch if Not Equal
            if (instr->operands.size() >= 3) {
                std::string reg1 = instr->operands[0];
                std::string reg2 = instr->operands[1];
                int offset = std::stoi(instr->operands[2]);

                if (registers[reg1] != registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BNE: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BNE: No branch taken." << std::endl;
                }
            }
        } else if (name == "blt") {
            // Branch if Less Than
            if (instr->operands.size() >= 3) {
                std::string reg1 = instr->operands[0];
                std::string reg2 = instr->operands[1];
                int offset = std::stoi(instr->operands[2]);

                if (registers[reg1] < registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BLT: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BLT: No branch taken." << std::endl;
                }
            }
        } else if (name == "bge") {
            // Branch if Greater or Equal
            if (instr->operands.size() >= 3) {
                std::string reg1 = instr->operands[0];
                std::string reg2 = instr->operands[1];
                int offset = std::stoi(instr->operands[2]);

                if (registers[reg1] >= registers[reg2]) {
                    pc += offset; // Branch taken
                    std::cout << "BGE: Branch taken to " << pc << "." << std::endl;
                } else {
                    std::cout << "BGE: No branch taken." << std::endl;
                }
            }
        } else {
            std::cout << "Instruction not recognized: " << name << std::endl;
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
                clock_cycle++;
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

// simulator.cpp

#include "simulator.h"

// Constructor
Simulator::Simulator(int num_runs)
    : clock_cycle(0), sim_ticks(0), clock_cycle_limit(num_runs), pc(0x0000), halt(false), stall_count(0) {
    pipeline_registers["Fetch"] = nullptr;
    pipeline_registers["Decode"] = nullptr;
    pipeline_registers["Execute"] = nullptr;
    pipeline_registers["Store"] = nullptr;
    registers["x10"] = 0;
    registers["x11"] = 0;
    registers["x12"] = 0;
    registers["x13"] = 0;
}

void Simulator::load_instructions_from_binary(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open binary file: " + filename);
    }

    uint32_t address = 0x0000;
    max_instruction_address = 0x0000;

    while (infile.peek() != std::ifstream::traits_type::eof()) {
        uint32_t instruction;
        infile.read(reinterpret_cast<char*>(&instruction), sizeof(instruction));

        if (!infile) {
            if (infile.eof()) break;
            throw std::runtime_error("Error reading from binary file: " + filename);
        }

        ram.write(address, instruction);
        max_instruction_address = address;
        address += 4;
    }
    infile.close();
}

void Simulator::fetch() {
    if (stall_count > 0) {
        std::cout << "Fetch: " << "Fetch stage is stalled." << std::endl;
        stall_count--;
        return;
    }

    if (pc <= max_instruction_address) {
        try {
            uint32_t instruction_value = ram.read(pc);
            std::string hex_value = to_hex_string(instruction_value);
            Instruction* instr = new Instruction(hex_value, instruction_value, {}, "Binary");

            instr->stage = "Fetch";
            instr->cycle_entered["Fetch"] = clock_cycle;
            pipeline_registers["Fetch"] = instr;
            event_list.push_back({instr->name, "Fetch", clock_cycle});
            std::cout << "Fetch: " << "Fetching instruction " << instr->name << std::endl;

            pc += 4;
        } catch (const std::out_of_range& e) {
            std::cerr << "PC out of bounds: " << e.what() << std::endl;
            halt = true;
            pipeline_registers["Fetch"] = nullptr;
        }
    } else {
        pipeline_registers["Fetch"] = nullptr;
        // halt = true;
    }
}

void Simulator::decode() {
    if (pipeline_registers["Fetch"] != nullptr) {
        Instruction* fetched_instr = pipeline_registers["Fetch"];
        uint32_t instruction_value = fetched_instr->binary;
        
        std::string decodedName = decoder.decodeInstruction(instruction_value);
        std::vector<std::string> operands = split_instruction(decodedName);
        
        fetched_instr->operands = operands;
        std::cout << "Decoder: " << decodedName << std::endl;
        pipeline_registers["Decode"] = fetched_instr;
        pipeline_registers["Fetch"] = nullptr;
    } else {
        std::cout << "Decoder: " << "No instruction to decode." << std::endl;
    }
}

void Simulator::execute() {
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
        pipeline_registers["Execute"] = instr;
        pipeline_registers["Decode"] = nullptr;
    } else {
        std::cout << "Execute: " << "No instruction to execute." << std::endl;
    }
}

void Simulator::store() {
    Instruction* instr = pipeline_registers["Execute"];
    if (instr) {
        instr->stage = "Store";
        instr->cycle_entered["Store"] = clock_cycle;
        event_list.push_back({instr->name, "Store", clock_cycle});
        pipeline_registers["Store"] = instr;
        pipeline_registers["Execute"] = nullptr;
        std::cout << "Store: " << "Storing " << instr->name << "." << std::endl;
    } else {
        std::cout << "Store: " << "No instruction to store." << std::endl;
    }
}

void Simulator::clean_event_list(Instruction* instr) {
    for (auto it = event_list.begin(); it != event_list.end();) {
        if (it->name == instr->name && it->stage == instr->stage) {
            it = event_list.erase(it);
        } else {
            ++it;
        }
    }
}

void Simulator::execute_instruction(Instruction* instr) {
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
    } if (name == "flw") {
        if (operands.size() >= 3) {
            std::string f_reg = operands[1];
            std::string addr_reg = operands[2];

            size_t left_paren = addr_reg.find('(');
            size_t right_paren = addr_reg.find(')');
            int offset = std::stoi(addr_reg.substr(0, left_paren));
            std::string base_reg = addr_reg.substr(left_paren + 1, right_paren - left_paren - 1);

            int base_addr = registers[base_reg];

            uint32_t raw_value = ram.read(base_addr + offset);
            float value;
            // std::memcpy(&value, &raw_value, sizeof(float));

            f_registers[f_reg] = value;
            std::cout << "Loaded " << value << " into " << f_reg << " from memory address " << std::hex << (base_addr + offset) << "." << std::dec << std::endl;
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

            // memory[base_addr + offset] = f_registers[f_reg];
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

            // registers[dest_reg] = memory[base_addr + offset]; // Load value from memory
            std::cout << "LW: Loaded " << registers[dest_reg] << " into " << dest_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
        }
    } else if (name == "sw") {
        // Store Word
        if (operands.size() >= 3) {
            std::string src_reg = operands[1];
            std::string addr_reg = operands[2];

            int base_addr = registers[addr_reg.substr(3, 3)];
            int offset = std::stoi(addr_reg.substr(0, addr_reg.find('(')));

            // memory[base_addr + offset] = registers[src_reg]; // Store value to memory
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
        std::cout << "Execute: " << "Unsupported instruction: " << name << std::endl;
    }
}

std::string Simulator::to_hex_string(uint32_t instruction) {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << instruction;
    return oss.str();
}

std::vector<std::string> Simulator::split_instruction(const std::string& instruction) {
    std::vector<std::string> operands;
    std::istringstream iss(instruction);
    std::string token;

    if (std::getline(iss, token, ' ')) {
        operands.push_back(token);
    }

    std::string remaining;
    if (std::getline(iss, remaining)) {
        std::istringstream operand_stream(remaining);
        while (std::getline(operand_stream, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            operands.push_back(token);
        }
    }

    return operands;
}

void Simulator::flush_pipeline() {
    for (auto& stage : pipeline_stages) {
        if (stage != "Store") pipeline_registers[stage] = nullptr;
    }
}

void Simulator::print_event_list() {
    std::cout << "\nEvent List at Cycle " << clock_cycle << ":" << std::endl;
    for (auto& event : event_list) {
        std::cout << "Instruction " << event.name << " in " << event.stage << " stage at cycle " << event.cycle << std::endl;
    }
}

void Simulator::print_instructions() {
    std::cout << "\nInstructions:" << std::endl;
    for (auto& instr : instructions) {
        std::cout << instr->name << " ";
        for (auto& op : instr->operands) {
            std::cout << op << " ";
        }
        std::cout << std::endl;
    }
}

void Simulator::print_pipeline_registers() {
    std::cout << "\nPipeline Registers:" << std::endl;
    for (auto& reg : pipeline_registers) {
        std::cout << reg.first << ": " << reg.second << std::endl;
    }
}

void Simulator::print_registers() {
    std::cout << "\nRegisters:" << std::endl;
    for (auto& reg : registers) {
        std::cout << reg.first << ": " << reg.second << std::endl;
    }
}

void Simulator::print_f_registers() {
    std::cout << "\nFloating Point Registers:" << std::endl;
    for (auto& reg : f_registers) {
        std::cout << reg.first << ": " << reg.second << std::endl;
    }
}

void Simulator::run() {
    while (!halt || pipeline_registers["Fetch"] || pipeline_registers["Decode"] || pipeline_registers["Execute"] || pipeline_registers["Store"]) {
        sim_ticks++;
        if(sim_ticks % 10 == 0) {
            clock_cycle++;
            sim_ticks = 0;
        }
        std::cout << "Cycle " << clock_cycle << "\n";
        std::cout << "--------------------------------------------------" << std::endl;
        store();
        execute();
        decode();
        fetch();
        std::cout << "--------------------------------------------------" << std::endl;

        if (stall_count > 0) stall_count--;

        // print_event_list();
        // print_instructions();
        // print_registers();
        if (clock_cycle_limit != 0 && clock_cycle >= clock_cycle_limit) break;
        if (halt) {
            flush_pipeline();
            break;
        }
    }
}

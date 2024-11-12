// simulator.cpp

#include "simulator.h"

// Constructor
Simulator::Simulator(int num_runs)
    : clock_cycle(0), sim_ticks(0), clock_cycle_limit(num_runs), pc(0x0000), halt(false), stall_count(0) {
    pipeline_registers["Fetch"] = nullptr;
    pipeline_registers["Decode"] = nullptr;
    pipeline_registers["Execute"] = nullptr;
    pipeline_registers["Store"] = nullptr;
    registers["sp"] = 0x3FF;                // Address of end of the stack
    registers["ra"] = 0;
    registers["s0"] = 0;
    registers["a0"] = 0;
    registers["zero"] = 0;
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

// Delay function to simulate clock cycle delays
void Simulator::delay_cycles(int cycle_count) {
    delay = cycle_count * 10;  // Convert cycle count to ticks 
}

void Simulator::fetch() {
    if (delay > 0) {
        if (pipeline_registers["Fetch"] != nullptr) {
            std::cout << "Fetch: Fetching instruction 0x" 
                      << pipeline_registers["Fetch"]->name 
                      << ", Delay remaining: " << delay << std::endl;
        } else {
            std::cout << "Fetch: Waiting for delay to complete. Delay remaining: " << delay << std::endl;
        }
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
            std::cout << "Fetch: Fetching instruction " << instr->name << std::endl;

            pc += 4;
            delay_cycles(2);  // Add 2 cycles (20 ticks) delay
        } catch (const std::out_of_range& e) {
            std::cerr << "PC out of bounds: " << e.what() << std::endl;
            halt = true;
            pipeline_registers["Fetch"] = nullptr;
        }
    } else {
        pipeline_registers["Fetch"] = nullptr;
    }
}

void Simulator::decode() {
    if (pipeline_registers["Fetch"] != nullptr) {
        decode_counter = 1;
        Instruction* fetched_instr = pipeline_registers["Fetch"];
        uint32_t instruction_value = fetched_instr->binary;

        // Decode the instruction name
        std::string decodedName = decoder.decodeInstruction(instruction_value);

        // Check if a delay is active and output the delay message if so
        if (delay > 0) {
            std::cout << "Decoder: " << decodedName << " Delay remaining: " << delay << std::endl;
            return;
        }

        // Decode operands if no delay
        std::vector<std::string> operands = split_instruction(decodedName);
        fetched_instr->operands = operands;
        std::cout << "Decoder: " << decodedName << std::endl;

        // Move instruction to Decode stage
        pipeline_registers["Decode"] = fetched_instr;
        pipeline_registers["Fetch"] = nullptr;
    } else {
        std::cout << "Decoder: No instruction to decode." << std::endl;
        decode_counter = 0;
    }
}

void Simulator::execute() {
    if (delay > 0) {
        std::cout << "Execute: Waiting for delay to complete. Delay remaining: " << delay << std::endl;
        return;
    }

    Instruction* instr = pipeline_registers["Decode"];
    if (instr) {
        excecute_counter = 1;
        instr->stage = "Execute";
        instr->cycle_entered["Execute"] = clock_cycle;

        execute_instruction(instr);

        int delay_amount = (instr->name.find("f") != std::string::npos) ? 5 :
                           ((instr->name == "sw" || instr->name == "lw" || instr->name == "fsw") ? 2 :
                           ((instr->name == "addi" || instr->name == "and" || instr->name == "or" || instr->name == "xori") ? 1 : 0));

        if (delay_amount > 0) delay_cycles(delay_amount);  // Delay as needed

        event_list.push_back({instr->name, "Execute", clock_cycle});
        pipeline_registers["Execute"] = instr;
        pipeline_registers["Decode"] = nullptr;
    } else {
        std::cout << "Execute: No instruction to execute." << std::endl;
        excecute_counter = 0;
    }
}

void Simulator::store() {
    static uint32_t effective_addr = 0;
    static std::string src_reg;

    // Handle delay countdown with improved output format
    if (delay > 0) {
        std::cout << "Store: Waiting to store " << registers[src_reg] 
                  << " into memory address " << effective_addr 
                  << ". Delay remaining: " << delay << std::endl;
        return;
    }

    Instruction* instr = pipeline_registers["Execute"];
    if (instr) {
        store_counter = 1;
        instr->stage = "Store";
        instr->cycle_entered["Store"] = clock_cycle;
        event_list.push_back({instr->name, "Store", clock_cycle});
        pipeline_registers["Store"] = instr;
        pipeline_registers["Execute"] = nullptr;

        std::vector<std::string>& operands = instr->operands;
        std::string name = operands[0];  // The instruction name

        if (name == "sw" || name == "fsw") {
            src_reg = operands[1];
            std::string addr_reg_offset = operands[2];

            size_t start = addr_reg_offset.find('(');
            size_t end = addr_reg_offset.find(')');
            std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
            int offset = std::stoi(addr_reg_offset.substr(0, start));

            int base_addr = registers[addr_reg];
            effective_addr = base_addr + offset;
            uint32_t value = registers[src_reg];
            ram.write(effective_addr, value);

            std::cout << "Store: Stored " << value << " into memory address " << effective_addr << "." << std::endl;
            delay_cycles(2);  // 2-cycle delay for store instructions
        } else {
            std::cout << "Store: No store instruction to process." << std::endl;
        }
    } else {
        std::cout << "Store: No instruction to store." << std::endl;
        store_counter = 0;
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

    if (name == "addi") {
        // Add Immediate
        if (operands.size() >= 4) {
            std::string dest_reg = operands[1];
            std::string src_reg = operands[2];
            int immediate = std::stoi(operands[3]);

            registers[dest_reg] = registers[src_reg] + immediate;
            std::cout << "Execute: " << "ADDI: Added " << immediate << " to " << src_reg << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
        }
    } else if (name == "add") {
        // Add Registers
        if (operands.size() >= 4) {
            std::string dest_reg = operands[1];
            std::string op0_reg = operands[2];
            std::string op1_reg = operands[3];

            uint32_t val0 = registers[op0_reg];
            uint32_t val1 = registers[op1_reg];

            registers[dest_reg] = val0 + val1;
            std::cout << "Execute: " << "ADD: " << op0_reg << ": " << val0 << " + " 
            << op1_reg << ": " << val1 << " = " << dest_reg << ": " << registers[dest_reg] <<   std::endl;
        }
    } else if (name == "sw") {
        // Store Word
        if (operands.size() >= 3) {
            std::string src_reg = operands[1];
            std::string addr_reg_offset = operands[2];
            size_t start = addr_reg_offset.find('(');
            size_t end = addr_reg_offset.find(')');

            std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
            int offset = std::stoi(addr_reg_offset.substr(0, start));

            int base_addr = registers[addr_reg];
            uint32_t effective_addr = base_addr + offset;
            uint32_t value = registers[src_reg];

            ram.write(effective_addr, value);
            std::cout<< "Execute: " << "SW: Stored " << registers[src_reg] << " into memory address " << effective_addr << "." << std::endl;
        }
    } else if (name == "lw") {
        // Load Word
        // std::cout << "Execute: LW: Currently not working" << std::endl;
        // return; // ISUEE WITH lw
        if (operands.size() >= 3) {
            std::string dest_reg = operands[1];
            std::string addr_reg_offset = operands[2];
            size_t start = addr_reg_offset.find('(');
            size_t end = addr_reg_offset.find(')');

            std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
            int offset = std::stoi(addr_reg_offset.substr(0, start));

            int base_addr = registers[addr_reg];
            uint32_t effective_addr = registers[addr_reg] + offset;

            registers[dest_reg] = ram.read(effective_addr);

            std::cout << "Execute: " << "LW: Loaded " << registers[dest_reg] << " into " << dest_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
        }
    } else if (name == "blt"){
        std::string less_reg = operands[1];
        std::string base_reg = operands[2];
        int immediate = std::stoi(operands[3]);
        if (registers[less_reg] < registers[base_reg]){
            pc = pc + immediate;
            std::cout << "Execute: " << "BLT: Jumped to " << immediate << registers[less_reg] << " < " << registers[base_reg] << std::endl;
        } else{
            std::cout << "Execute: " << "BLT: Didnt jump to " << immediate << " not " << registers[less_reg] << " < " << registers[base_reg] << std::endl;
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
    } else if (name == "flw") {
        if (operands.size() >= 3) {
            std::string f_reg = operands[1];
            std::string addr_reg_offset = operands[2];

            size_t start = addr_reg_offset.find('(');
            size_t end = addr_reg_offset.find(')');
            std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
            int offset = std::stoi(addr_reg_offset.substr(0, start));

            int base_addr = registers[addr_reg];
            uint32_t effective_addr = registers[addr_reg] + offset;

            registers[f_reg] = ram.read(effective_addr);

            std::cout << "Loaded " << registers[f_reg] << " into " << f_reg << " from memory address " << std::hex << (effective_addr) << "." << std::dec << std::endl;
        }
    } else if (name == "fadd.s") {
        // Floating point addition
        if (operands.size() >= 4) {
            std::string dest_reg = operands[1]; // Destination register
            std::string op0_reg = operands[2]; // First source register
            std::string op1_reg = operands[3]; // Second source register

            uint32_t val0 = registers[op0_reg];
            uint32_t val1 = registers[op1_reg];

            registers[dest_reg] = val0 + val1;
            std::cout << "Execute: " << "FADD.s: " << op0_reg << ": " << val0 << " + " 
            << op1_reg << ": " << val1 << " = " << dest_reg << ": " << registers[dest_reg] <<   std::endl;
        }
    } else if (name == "fsw") {
        // Store floating point word from register into memory
        if (operands.size() >= 3) {
            std::string f_reg = operands[1]; // e.g., "f2"
            std::string addr_reg_offset = operands[2]; // e.g., "0(x12)"

            size_t start = addr_reg_offset.find('(');
            size_t end = addr_reg_offset.find(')');
            std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
            int offset = std::stoi(addr_reg_offset.substr(0, start));

            int base_addr = registers[addr_reg];
            uint32_t effective_addr = registers[addr_reg] + offset;

            uint32_t value = registers[f_reg];

            ram.write(effective_addr, value);
            std::cout<< "Execute: " << "FSW: Stored " << registers[f_reg] << " into memory address " << effective_addr << "." << std::endl;
        }
    } else if (name == "jal"){
        int immediate = std::stoi(operands[2]);
        registers["ra"] = pc + 4;
        pc = pc + immediate;
        if (immediate*immediate > 0)
            pipeline_registers["Fetch"] = nullptr;
    }
    
    else if (name == "auipc") {
        // Add Upper Immediate to PC
        if (operands.size() >= 3) {
            std::string reg = operands[1];
            int immediate = std::stoi(operands[2], nullptr, 16); // Convert hex string to integer
            registers[reg] = pc + (immediate << 12); // Shift left by 12 bits
            std::cout << "AUIPC: Loaded " << registers[reg] << " into " << reg << " with immediate " << immediate << "." << std::endl;
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
        if(delay > 0){
            delay--;
        }

        // print_event_list();
        // print_instructions();
        // print_registers();
        if (clock_cycle_limit != 0 && clock_cycle >= clock_cycle_limit) break;
        if (halt) {
            flush_pipeline();
            break;
        }
        if (delay == 0 &&
            clock_cycle > 10 &&
            store_counter == 0 && 
            excecute_counter == 0 && 
            decode_counter == 0) {
            
            std::cout << "Simulation completed at clock cycle: " << clock_cycle << std::endl;
            break;  // End the simulation
        }
    }
}

#include "core.h"
#include "membus.h"

// Constructor
Core::Core(int start_pc, int core_id, uint32_t initial_sp)
    : clock_cycle(0), sim_ticks(0), pc(start_pc), halt(false), stall_count(0), ram(nullptr), core_id(core_id) {
    complete = 0;
    pipeline_registers["Fetch"] = nullptr;
    pipeline_registers["Decode"] = nullptr;
    pipeline_registers["Execute"] = nullptr;
    pipeline_registers["Store"] = nullptr;
    fetching_active = 1;

    registers["sp"] = initial_sp; // Use the input value for the stack pointer
    registers["ra"] = 0;
    registers["s0"] = 0;
    registers["a0"] = 0;
    registers["zero"] = 0;
}

void Core::set_ram(RAM* ram_ptr) {
    ram = ram_ptr;
}

void Core::set_membus(Membus* membus_ptr) {
    membus = membus_ptr;
}

// Delay function to simulate clock cycle delays
int Core::delay_cycles(int cycle_count) {
    return cycle_count * 10;  // Convert cycle count to ticks 
}

void Core::fetch() {
    if (pipeline_registers["Decode"]){
        std::cout << "Fetch: Decode is busy." << std::endl;
        return;
    }

    if (pc < start_address) {
        throw std::out_of_range("PC out of bounds.");
        return;
    } else if (pc <= max_instruction_address) {
        try {
            std::vector<uint32_t> returnValues = membus->read(core_id, pc, false); // ram->read(pc, false);
            uint32_t instruction_value = returnValues[0];

            if (returnValues[0] == UINT32_MAX){
                fetching_active = 1;
                std::cout << "Fetch: Waiting for instruction to load. " << "Cycles remaining: " << returnValues[2]  << std::endl;
                return;
            }
            fetching_active = 0;
            instruction_count++;
            std::string hex_value = to_hex_string(instruction_value);
            Instruction* instr = new Instruction(hex_value, instruction_value, {}, "Binary");

            instr->stage = "Fetch";
            instr->cycle_entered["Fetch"] = clock_cycle;
            pipeline_registers["Fetch"] = instr;
            event_list.push_back({instr->name, "Fetch", clock_cycle});
            std::cout << "Fetch: Fetching instruction " << instr->name << "." << std::endl;
            pipeline_registers["Decode"] = instr;

            pc += 4;
        } catch (const std::out_of_range& e) {
            std::cerr << "PC out of bounds: " << e.what() << std::endl;
            halt = true;
            pipeline_registers["Fetch"] = nullptr;
        }
    } else {
        fetching_active = 0;
        std::cout << "Fetch: No instructions to fetch." << std::endl;
        pipeline_registers["Fetch"] = nullptr;
        halt = true; // No more instructions to fetch
        return;
    }
}

void Core::decode() {
    if (pipeline_registers["Decode"]) {
        decode_counter = 1;
        Instruction* fetched_instr = pipeline_registers["Decode"];
        uint32_t instruction_value = fetched_instr->binary;

        // Decode the instruction name
        std::string decodedName = decoder.decodeInstruction(instruction_value);

        // Decode operands if no delay
        std::vector<std::string> operands = split_instruction(decodedName);
        fetched_instr->operands = operands;

        if (decodedName == "Unknown"){
            std::cout << "Decoder: End of program reached" << std::endl;
            return;
        }

        if (!pipeline_registers["Execute"]){
            std::cout << "Decoder: " << decodedName << std::endl;

            // Move instruction to Decode stage
            pipeline_registers["Decode"] = nullptr;
            pipeline_registers["Execute"] = fetched_instr;
        }
        else{
            std::cout << "Decoder: Execute is busy." << std::endl;
        }
    } 
    else {
        std::cout << "Decoder: No instruction to decode." << std::endl;
        decode_counter = 0;
    }
}

void Core::execute() { 
    Instruction* instr = pipeline_registers["Execute"];
    if (instr) {
        // If execute_delay_remaining == 0, initialize it based on instruction type
        if (instr->execute_delay == 0 && instr->store_delay == 0 && !execute_delay_complete) {
            instr->stage = "Execute";
            instr->cycle_entered["Execute"] = clock_cycle;

            // Extract operands and validate
            std::vector<std::string>& operands = instr->operands;

            if (operands.empty()) {
                std::cout << "Execute: Invalid instruction, no operands provided." << std::endl;
                return;
            }

            // Execute based on instruction name
            std::string name = operands[0];  // The first operand is the instruction name

            // Handle store instructions
            // if (name == "sw" || name == "fsw"){
            //     if (!pipeline_registers["Store"]){
            //         pipeline_registers["Store"] = instr;
            //         pipeline_registers["Execute"] = nullptr;
            //         std::cout << "Execute: Instruction " << name << " sent to Store stage." << std::endl;
            //         return;
            //     } else {
            //         std::cout << "Execute: Store stage busy, cannot send instruction " << name << std::endl;
            //         return;
            //     }
            // }

            // Determine delay based on instruction type
            int delay_amount = (name == "fadd.s" || name == "fsub.s" || name == "flw" || name == "fsw") ? 5 :
                               ((name == "addi" || name == "and" || name == "or" || name == "xori" ||
                                 name == "slli" || name == "blt" || name == "jal"|| name == "jalr" ||
                                 name == "lw" || name == "sw") ? 1 : 0);

            instr->execute_delay = delay_amount;
            if (instr->execute_delay > 0) {
                std::cout << "Execute: Instruction " << name << " delay remaining: " << instr->execute_delay << std::endl;
                return; // Do not proceed further this cycle
            }
        } else {
            // Decrement execute_delay_remaining
            if (instr->store_delay == 0){
                instr->execute_delay--;
                if (instr->execute_delay > 0) {
                    // Delay not yet expired, keep instruction in Execute stage
                    std::cout << "Execute: Instruction " << instr->operands[0] << " delay remaining: " << instr->execute_delay << std::endl;
                    return;
                }
            }
            else{
                instr->store_delay--;
                return;
            }
        }
        execute_delay_complete = true;
        // Now execute_delay_remaining == 0, proceed to execute instruction
        execute_instruction(instr, instr->operands[0], instr->operands);

        event_list.push_back({instr->name, "Execute", clock_cycle});

    } else {
        std::cout << "Execute: No instruction to execute." << std::endl;
    }
}

void Core::store() {

    Instruction* instr = pipeline_registers["Store"];
    if (instr) {
        store_counter = 1;
        instr->stage = "Store";
        instr->cycle_entered["Store"] = clock_cycle;
        event_list.push_back({instr->name, "Store", clock_cycle});

        std::vector<std::string>& operands = instr->operands;
        std::string name = operands[0];

        int added_delay = (name == "fsw") ? 5: 1;

        store_instruction(name, operands, added_delay);

    } else {
        std::cout << "Store: No instruction to store." << std::endl;
        return;
    }

}

void Core::store_instruction(std::string name, std::vector<std::string> operands, int added_delay){
    static uint32_t effective_addr = 0;
    static std::string src_reg;

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
        
        float floatValue;
        std::memcpy(&floatValue, &value, sizeof(floatValue));

        std::vector<uint32_t> returnValues = membus->write(core_id, effective_addr, value, 0, false); // ram->write(effective_addr, value, 0, false);

        if (returnValues[0] && returnValues[0] != UINT32_MAX){
            if (name == "fsw")
                std::cout << "Store: " << name << ": Store " << floatValue << " into memory address " << effective_addr << " successful." << std::endl;
            else
                std::cout << "Store: " << name << ": Store " << value << " into memory address " << effective_addr << " successful." << std::endl;
            hold_registers[addr_reg] = false;
        }
        else {
            std::cout << "Store: Store operation pending on address " << effective_addr << " Cycles remaining: " << returnValues[1] << std::endl;
            hold_registers[addr_reg] = true;
            return;
        }

    } else {
        std::cout << "Store: No store instruction to process." << std::endl;
        return;
    }
    pipeline_registers["Store"] = nullptr;
    store_delay_complete = 0;
}

void Core::clean_event_list(Instruction* instr) {
    for (auto it = event_list.begin(); it != event_list.end();) {
        if (it->name == instr->name && it->stage == instr->stage) {
            it = event_list.erase(it);
        } else {
            ++it;
        }
    }
}

void Core::execute_instruction(Instruction* instr, std::string name, std::vector<std::string> operands) {

    if (name == "addi") {
        // Add Immediate
        std::string dest_reg = operands[1];
        std::string src_reg = operands[2];
        int immediate = std::stoi(operands[3]);

        registers[dest_reg] = registers[src_reg] + immediate;
        std::cout << "Execute: " << "ADDI: Added " << immediate << " to " << src_reg << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
    } else if (name == "add") {
        // Add Registers
        std::string dest_reg = operands[1];
        std::string op0_reg = operands[2];
        std::string op1_reg = operands[3];

        uint32_t val0 = registers[op0_reg];
        uint32_t val1 = registers[op1_reg];

        registers[dest_reg] = val0 + val1;
        std::cout << "Execute: " << "ADD: " << op0_reg << ": " << val0 << " + " 
        << op1_reg << ": " << val1 << " = " << dest_reg << ": " << registers[dest_reg] <<   std::endl;
    } else if (name == "lw" || name == "flw") {
        // Load Word
        std::string dest_reg = operands[1];
        std::string addr_reg_offset = operands[2];
        size_t start = addr_reg_offset.find('(');
        size_t end = addr_reg_offset.find(')');

        std::string addr_reg = addr_reg_offset.substr(start + 1, end - start - 1);
        int offset = std::stoi(addr_reg_offset.substr(0, start));

        int base_addr = registers[addr_reg];
        uint32_t effective_addr = registers[addr_reg] + offset;

        std::vector<uint32_t> returnValues = membus->read(core_id, effective_addr, false); // ram->read(effective_addr, false);

        if (hold_registers[dest_reg]){
            std::cout <<  "Execute: Holding register " << dest_reg << "." << std::endl;
            return;
        }

        else if (returnValues[0] != UINT32_MAX && returnValues[0] != UINT32_MAX-1){
            registers[dest_reg] = returnValues[0];
            std::cout << "Execute: " << name << ": Loaded " << registers[dest_reg] << " into " << dest_reg << " from memory address " << (base_addr + offset) << "." << std::endl;
        }
        else if (returnValues[0] == UINT32_MAX-1){
            registers[dest_reg] = returnValues[0];
            std::cout << "Execute: " << name << ": Waiting for other core to finish." << std::endl;
            return;
        }
        else if (returnValues[1]) {
            std::cout << "Execute: Store operation pending on address " << effective_addr << std::endl;
            instr->store_delay = returnValues[1];
            return;
        }
        else if (returnValues[2]){
            std::cout << "Execute: Waiting to load from " << effective_addr
                  << ". Delay remaining: " << returnValues[2] << std::endl;
            return;
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
        std::string dest_reg = operands[1];
        std::string src_reg = operands[2];
        int shift_amount = std::stoi(operands[3]);

        registers[dest_reg] = registers[src_reg] << shift_amount;
        std::cout << "Execute: SLLI: Shifted " << src_reg << " left by " << shift_amount << ", result in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
    } else if (name == "fadd.s") {
    // Floating point addition
    std::string dest_reg = operands[1]; // Destination register
    std::string op0_reg = operands[2]; // First source register
    std::string op1_reg = operands[3]; // Second source register

    uint32_t val0 = registers[op0_reg];
    uint32_t val1 = registers[op1_reg];

    // Convert uint32_t to float
    float fval0, fval1;
    std::memcpy(&fval0, &val0, sizeof(fval0));
    std::memcpy(&fval1, &val1, sizeof(fval1));

    // Perform floating-point addition
    float fresult = fval0 + fval1;

    // Convert float result back to uint32_t
    uint32_t result;
    std::memcpy(&result, &fresult, sizeof(result));

    registers[dest_reg] = result;

    // Debug output
    std::cout << "Execute: FADD.s: " << op0_reg << ": " << fval0 << " + " 
              << op1_reg << ": " << fval1 << " = " << dest_reg << ": " << fresult << std::endl;
    } else if (name == "fsub.s") {
        // Floating point subtraction
        std::string dest_reg = operands[1]; // Destination register
        std::string op0_reg = operands[2]; // First source register
        std::string op1_reg = operands[3]; // Second source register

        uint32_t val0 = registers[op0_reg];
        uint32_t val1 = registers[op1_reg];

        // Convert uint32_t to float
        float fval0, fval1;
        std::memcpy(&fval0, &val0, sizeof(fval0));
        std::memcpy(&fval1, &val1, sizeof(fval1));

        // Perform floating-point subtraction
        float fresult = fval0 - fval1;

        // Convert float result back to uint32_t
        uint32_t result;
        std::memcpy(&result, &fresult, sizeof(result));

        registers[dest_reg] = result;

        // Debug output
        std::cout << "Execute: FSUB.s: " << op0_reg << ": " << fval0 << " - " 
                << op1_reg << ": " << fval1 << " = " << dest_reg << ": " << fresult << std::endl;
    } else if (name == "jal") {
        int offset = std::stoi(operands[2]);
        registers["ra"] = pc + 4; // Save return address
        pc = pc + offset;

        if (abs(offset) > 0)
            flush_pipeline(); // Clear the pipeline
        
        std::cout << "Execute: JAL: Jumped " << offset << " to instruction " << pc << "." << std::endl;
    }

    else if (name == "auipc") {
        // Add Upper Immediate to PC
        std::string reg = operands[1];
        int immediate = std::stoi(operands[2], nullptr, 16); // Convert hex string to integer
        registers[reg] = pc + (immediate << 12); // Shift left by 12 bits
        std::cout << "Execute: AUIPC: Loaded " << registers[reg] << " into " << reg << " with immediate " << immediate << "." << std::endl;
    } else if (name == "jalr") {
        // Jump and Link Register
        std::string dest_reg = operands[1];
        int offset = std::stoi(operands[2]);

        pc += offset; // Jump to the address
        registers[dest_reg] = pc; // Save return address
        std::cout << "Execute: JALR: Jumped to address " << pc << ", return address in " << dest_reg << ": " << registers[dest_reg] << "." << std::endl;
    } else if (name == "beq") {
        // Branch if Equal
        std::string reg1 = operands[1];
        std::string reg2 = operands[2];
        int offset = std::stoi(operands[3]);

        if (registers[reg1] == registers[reg2]) {
            pc += offset; // Branch taken
            std::cout << "Execute: BEQ: Branch taken to " << pc << "." << std::endl;
        } else {
            std::cout << "Execute: BEQ: No branch taken." << std::endl;
        }
    } else if (name == "bne") {
        // Branch if Not Equal
        std::string reg1 = operands[1];
        std::string reg2 = operands[2];
        int offset = std::stoi(operands[3]);

        if (registers[reg1] != registers[reg2]) {
            pc += offset; // Branch taken
            std::cout << "Execute: BNE: Branch taken to " << pc << "." << std::endl;
        } else {
            std::cout << "Execute: BNE: No branch taken." << std::endl;
        }
    } else if (name == "sw" || name == "fsw"){
        if (!pipeline_registers["Store"]){
            pipeline_registers["Store"] = instr;
            pipeline_registers["Execute"] = nullptr;
            std::cout << "Execute: Instruction " << name << " sent to Store stage." << std::endl;
            return;
        } else {
            std::cout << "Execute: Store stage busy, cannot send instruction " << name << std::endl;
            return;
        }
    } else {
        std::cout << "Execute: " << "Unsupported instruction: " << name << std::endl;
        return;
    }
    pipeline_registers["Execute"] = nullptr;
    execute_delay_complete = 0;
}

std::string Core::to_hex_string(uint32_t instruction) {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << instruction;
    return oss.str();
}

std::vector<std::string> Core::split_instruction(const std::string& instruction) {
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

void Core::flush_pipeline() {
    for (auto& stage : pipeline_stages) {
        if (stage == "Fetch" || stage == "Decode") pipeline_registers[stage] = nullptr;
    }
}

void Core::print_event_list() {
    std::cout << "\nEvent List at Cycle " << clock_cycle << ":" << std::endl;
    for (auto& event : event_list) {
        std::cout << "Instruction " << event.name << " in " << event.stage << " stage at cycle " << event.cycle << std::endl;
    }
}

void Core::print_instructions() {
    std::cout << "\nInstructions:" << std::endl;
    for (auto& instr : instructions) {
        std::cout << instr->name << " ";
        for (auto& op : instr->operands) {
            std::cout << op << " ";
        }
        std::cout << std::endl;
    }
}

void Core::print_pipeline_registers() {
    std::cout << "\nPipeline Registers:" << std::endl;
    for (auto& reg : pipeline_registers) {
        std::cout << reg.first << ": " << reg.second << std::endl;
    }
}

void Core::print_registers() {
    std::cout << "\nRegisters:" << std::endl;
    for (auto& reg : registers) {
        std::cout << reg.first << ": " << reg.second << std::endl;
    }
}

bool Core::is_halted() const {
    return halt;
}

bool Core::is_complete() const {
    return !pipeline_registers.at("Fetch") &&
           !pipeline_registers.at("Decode") &&
           !pipeline_registers.at("Execute") &&
           !pipeline_registers.at("Store") &&
           !fetching_active;
}

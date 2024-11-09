#include "decoder.h"

InstructionMap InstructionMapping = 
{
    {
        OPCODE_LOAD, 
        {
            {0b000, "lb"},
            {0b001, "lh"},
            {0b010, "lw"},
            {0b100, "lbu"},
            {0b101, "lhu"},
            {0b110, "flw"}
        }
    },
    {
        OPCODE_I_TYPE, 
        {
            {0b000, "addi"},
            {0b001, "slli"},
            {0b010, "slti"},
            {0b011, "sltiu"},
            {0b100, "xori"},
            {0b101, Funct7Map{{0b0000000, "srli"}, {0b0100000, "srai"}}},
            {0b110, "ori"},
            {0b111, "andi"}
        }
    },
    {
        OPCODE_AUIPC, 
        {
            {NO_FUNCT3, "auipc"}
        }
    },
    {
        OPCODE_S_TYPE, 
        {
            {0b000, "sb"},
            {0b001, "sh"},
            {0b010, "sw"}
        }
    },
    {
        OPCODE_S_TYPE_FP, 
        {
            {0b010, "fsw"}
        }
    },
    {
        OPCODE_R_TYPE, 
        {
            {0b000, Funct7Map{
                {0b0000000, "add"}, 
                {0b0100000, "sub"}, 
                {0b0000001, "mul"},
                }
            },  // Adding fadd.s to the R-type mapping
            {0b001, Funct7Map{{0b0000000, "sll"}, {0b0000001, "mulh"}}},
            {0b010, Funct7Map{{0b0000000, "slt"}, {0b0000001, "mulhsu"}}},
            {0b011, Funct7Map{{0b0000000, "sltu"}, {0b0000001, "mulhu"}}},
            {0b100, Funct7Map{{0b0000000, "xor"}, {0b0000001, "div"}}},
            {0b101, Funct7Map{{0b0000000, "srl"}, {0b0100000, "sra"}, {0b0000001, "divu"}}},
            {0b110, Funct7Map{{0b0000000, "or"}, {0b0000001, "rem"}}},
            {0b111, Funct7Map{{0b0000000, "and"}, {0b0000001, "remu"}}},
        }
    },
    {
        OPCODE_LUI, 
        {
            {NO_FUNCT3, "lui"}
        }
    },
    {
        OPCODE_SB_TYPE, 
        {
            {0b000, "beq"},
            {0b001, "bne"},
            {0b100, "blt"},
            {0b101, "bge"},
            {0b110, "bltu"},
            {0b111, "bgeu"}
        }
    },
    {
        OPCODE_JALR, 
        {
            {0b000, "jalr"}
        }
    },
    {
        OPCODE_JAL,
        {
            {NO_FUNCT3, "jal"}
        }
    },
    {
        OPTCODE_FP,
        {
            {0b000, Funct7Map{
                {0b0000000, "fadd.s"},
                {0b0000100, "fsub.s"},
                {0b0001000, "fmul.s"},
                {0b0001100, "fdiv.s"},
                {0b0101100, "fsqrt.s"},
                // Add other single-precision floating-point operations here
                }
            },
            {0b001, Funct7Map{
                {0b0000000, "fadd.d"},
                {0b0000100, "fsub.d"},
                {0b0001000, "fmul.d"},
                {0b0001100, "fdiv.d"},
                {0b0101100, "fsqrt.d"},
                // Add other double-precision floating-point operations here
                }
            },
        }
    }
        
};


// Control signals mapping
std::unordered_map<uint8_t, ControlSignals> ControlInstructions = 
{
    {OPCODE_LOAD, {true, true, false, true, true, false, false, false, false}},
    {OPCODE_LOAD_FP, {true, true, false, true, true, false, false, false, false}},
    {OPCODE_I_TYPE, {true, false, false, false, true, false, false, false, false}},
    {OPCODE_AUIPC, {true, false, false, false, true, false, false, false, false}},
    {OPCODE_S_TYPE, {false, false, true, false, true, false, false, false, false}},
    {OPCODE_S_TYPE_FP, {false, false, true, false, true, false, false, false, false}},
    {OPCODE_R_TYPE, {true, false, false, false, false, false, false, false, false}},
    {OPCODE_LUI, {true, false, false, false, true, false, false, false, false}},
    {OPCODE_SB_TYPE, {false, false, false, false, false, true, false, false, false}},
    {OPCODE_JALR, {true, false, false, false, true, false, true, true, false}},
    {OPCODE_JAL, {true, false, false, false, true, false, true, false, false}}
};

// Constructor for Simulator
Decoder::Decoder() {}

// Decode instruction based on opcode
std::string Decoder::decodeInstruction(uint32_t instruction) {
    uint8_t opcode = getOpcode(instruction);
    ControlSignals signals = ControlInstructions[opcode];
    InstructionVariables vars;
    std::vector<std::string> printStatement;

    // Determine the format and fill in `vars` based on opcode
    InstructionFormat format;

    switch(opcode) {
        case OPCODE_LOAD:
        case OPCODE_LOAD_FP:
        case OPCODE_I_TYPE:
        case OPCODE_JALR:
            format = FORMAT_I;
            vars.rs1 = getRS1(instruction);
            vars.rd = getRD(instruction);
            vars.funct3 = getFunct3(instruction);
            vars.immediate = getImmediate(instruction);
            break;
        case OPCODE_AUIPC:
        case OPCODE_LUI:
            format = FORMAT_U;
            vars.rd = getRD(instruction);
            vars.immediate = getImmediate(instruction);
            break;
        case OPCODE_S_TYPE:
            format = FORMAT_S;
            vars.rs1 = getRS1(instruction);
            vars.rs2 = getRS2(instruction);
            vars.funct3 = getFunct3(instruction);
            vars.immediate = getImmediate(instruction);
            break;
        case OPCODE_SB_TYPE:
            format = FORMAT_B;
            vars.rs1 = getRS1(instruction);
            vars.rs2 = getRS2(instruction);
            vars.funct3 = getFunct3(instruction);
            vars.immediate = getImmediate(instruction);
            break;
        case OPTCODE_FP:
        case OPCODE_R_TYPE:
            format = FORMAT_R;
            vars.rs1 = getRS1(instruction);
            vars.rs2 = getRS2(instruction);
            vars.rd = getRD(instruction);
            vars.funct3 = getFunct3(instruction);
            vars.funct7 = getFunct7(instruction);

            break;
        case OPCODE_JAL:
            format = FORMAT_J;
            vars.rd = getRD(instruction);
            vars.immediate = getImmediate(instruction);
            break;
        default:
            std::cout << "Unknown opcode: " << std::bitset<7>(opcode) << std::endl;
            return "Unknown";
    }

    std::variant<std::string, Funct7Map> instructionType = InstructionMapping[opcode][vars.funct3];
    std::string decodedInstructionName;

    if (instructionType.index() == 0) {
        decodedInstructionName = std::get<std::string>(instructionType);
    } else {
        Funct7Map funct7Map = std::get<Funct7Map>(instructionType);
        decodedInstructionName = funct7Map[vars.funct7];
    }

    addRegisters(vars, printStatement, opcode);
    // printControlSignals(signals);

    // Build the full decoded instruction as a string
    std::string fullDecodedInstruction;
    for (size_t i = 0; i < printStatement.size(); ++i) {
        fullDecodedInstruction += printStatement[i];
        if (i != printStatement.size() - 1) {
            fullDecodedInstruction += " ";  // Add space between each part of the instruction
        }
    }

    decodedInstructionName += " " + fullDecodedInstruction; // Append full decoded instruction to the name

    return decodedInstructionName;
}

// Helper functions for extracting instruction fields
uint32_t Decoder::getOpcode(uint32_t instruction) {
    return instruction & 0x7F;
}

uint32_t Decoder::getRS1(uint32_t instruction) {
    return (instruction >> 15) & 0x1F;
}

uint32_t Decoder::getRS2(uint32_t instruction) {
    return (instruction >> 20) & 0x1F;
}

uint32_t Decoder::getRD(uint32_t instruction) {
    return (instruction >> 7) & 0x1F;
}

uint32_t Decoder::getFunct3(uint32_t instruction) {
    return (instruction >> 12) & 0x7;
}

uint32_t Decoder::getFunct7(uint32_t instruction) {
    return (instruction >> 25) & 0x7F;
}

int32_t Decoder::getImmediate(uint32_t instruction) {
    uint32_t opcode = getOpcode(instruction);
    int32_t imm = 0;

    switch(opcode) {
        case OPCODE_LOAD:
        case OPCODE_I_TYPE:
        case OPCODE_JALR:
            imm = (instruction >> 20) & 0xFFF;
            if (imm & 0x800) imm |= 0xFFFFF000;
            break;
        case OPCODE_S_TYPE:
            imm = ((instruction >> 25) & 0x7F) << 5 | (instruction >> 7) & 0x1F;
            if (imm & 0x800) imm |= 0xFFFFF000;
            break;
        case OPCODE_SB_TYPE:
            imm = ((instruction >> 31) & 0x1) << 12 |
                  ((instruction >> 7) & 0x1) << 11 |
                  ((instruction >> 25) & 0x3F) << 5 |
                  ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x1000) imm |= 0xFFFFE000;
            break;
        case OPCODE_AUIPC:
        case OPCODE_LUI:
            imm = instruction >> 12;
            if (imm & 0x80000) imm |= 0xFFF00000;
            break;
        case OPCODE_JAL:
            imm = ((instruction >> 31) & 0x1) << 20 |
                  ((instruction >> 21) & 0x3FF) << 1 |
                  ((instruction >> 20) & 0x1) << 11 |
                  ((instruction >> 12) & 0xFF) << 12;
            if (imm & 0x100000) imm |= 0xFFE00000;
            break;
        default:
            imm = 0;
            break;
    }
    return imm;
}

void Decoder::printOperands(int op1, int op2, int op3, std::vector<std::string>& printStatement,
                              std::vector<bool> isReg, bool isImmediateLast) {
    std::vector<std::string> operands;
    if (op1 == 2) 
        operands.push_back("sp");
    else if (op1 != NO_REGISTER) 
        operands.push_back((isReg[0] ? "x" : "") + std::to_string(op1));
    if (op2 == 2) 
        operands.push_back("sp");
    else if (op2 != NO_REGISTER && op2 != NO_IMMEDIATE) 
        operands.push_back((isReg[1] ? "x" : "") + std::to_string(op2));
    if (op3 != NO_REGISTER && op3 != NO_IMMEDIATE) {
        std::string operand = isImmediateLast ? std::to_string(op3) : (isReg[2] ? "x" : "") + std::to_string(op3);
        operands.push_back(operand);
    }

    for (size_t i = 0; i < operands.size(); ++i) {
        printStatement.push_back(operands[i]);
        if (i != operands.size() - 1) {
            printStatement.back() += ",";
        }
    }
}

void Decoder::addRegisters(InstructionVariables& vars, std::vector<std::string>& printStatement, uint8_t opcode) {
    switch (opcode) {
        case OPCODE_R_TYPE:
            printOperands(vars.rd, vars.rs1, vars.rs2, printStatement, {true, true, true});
            break;
        case OPCODE_I_TYPE:
            printOperands(vars.rd, vars.rs1, vars.immediate, printStatement, {true, true, false}, true);
            break;
        case OPCODE_LOAD:
        case OPCODE_LOAD_FP:
            if (vars.rd != NO_REGISTER) printStatement.push_back("x" + std::to_string(vars.rd) + ",");
            if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER)
                printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
            break;
        case OPCODE_S_TYPE:
        case OPCODE_S_TYPE_FP:
            if (vars.rs2 != NO_REGISTER) printStatement.push_back("x" + std::to_string(vars.rs2) + ",");
            if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER)
                printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
            break;
        case OPCODE_SB_TYPE:
            printOperands(vars.rs1, vars.rs2, vars.immediate, printStatement, {true, true, false}, true);
            break;
        case OPCODE_LUI:
        case OPCODE_AUIPC:
            printOperands(vars.rd, vars.immediate, NO_IMMEDIATE, printStatement, {true, false, false}, true);
            break;
        case OPCODE_JAL:
            printOperands(vars.rd, vars.immediate, NO_IMMEDIATE, printStatement, {true, false, false}, true);
            break;
        case OPCODE_JALR:
            if (vars.rd != NO_REGISTER) printStatement.push_back("x" + std::to_string(vars.rd) + ",");
            if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER)
                printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
            break;
        default:
            break;
    }
}

void Decoder::printControlSignals(const ControlSignals& signals) {
    std::cout << "RegWrite = " << signals.RegWrite << " "
              << "MemRead = " << signals.MemRead << " "
              << "MemWrite = " << signals.MemWrite << " "
              << "MemToReg = " << signals.MemToReg << "\n"
              << "ALUSrc = " << signals.ALUSrc << " "
              << "Branch = " << signals.Branch << " "
              << "Jump = " << signals.Jump << " "
              << "JumpReg = " << signals.JumpReg << " "
              << "Zero = " << signals.Zero << "\n";
}

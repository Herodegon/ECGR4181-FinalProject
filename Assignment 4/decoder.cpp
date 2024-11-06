#include "decoder.h"

// Constructor for Simulator
Simulator::Simulator() {}

// Decode instruction based on opcode
void Simulator::decodeInstruction(uint32_t instruction) {
    uint8_t opcode = getOpcode(instruction);
    ControlSignals signals = ControlInstructions[opcode];
    InstructionVariables vars;
    std::vector<std::string> printStatement;

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
            return;
    }

    std::variant<std::string, Funct7Map> instructionType = Instructions[opcode][vars.funct3];
    if (instructionType.index() == 0) {
        std::string tempStr = std::get<std::string>(instructionType);
        printStatement.push_back(tempStr);
    } else {
        Funct7Map funct7Map = std::get<Funct7Map>(instructionType);
        std::string tempStr = funct7Map[vars.funct7];
        printStatement.push_back(tempStr);
    }

    addRegisters(vars, printStatement, opcode);
    printControlSignals(signals);

    for (size_t i = 0; i < printStatement.size(); ++i) {
        std::cout << printStatement[i];
        if (i != printStatement.size() - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

// Helper functions for extracting instruction fields
uint32_t Simulator::getOpcode(uint32_t instruction) {
    return instruction & 0x7F;
}

uint32_t Simulator::getRS1(uint32_t instruction) {
    return (instruction >> 15) & 0x1F;
}

uint32_t Simulator::getRS2(uint32_t instruction) {
    return (instruction >> 20) & 0x1F;
}

uint32_t Simulator::getRD(uint32_t instruction) {
    return (instruction >> 7) & 0x1F;
}

uint32_t Simulator::getFunct3(uint32_t instruction) {
    return (instruction >> 12) & 0x7;
}

uint32_t Simulator::getFunct7(uint32_t instruction) {
    return (instruction >> 25) & 0x7F;
}

int32_t Simulator::getImmediate(uint32_t instruction) {
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

void Simulator::printOperands(int op1, int op2, int op3, std::vector<std::string>& printStatement,
                              std::vector<bool> isReg, bool isImmediateLast) {
    std::vector<std::string> operands;
    if (op1 != NO_REGISTER) operands.push_back((isReg[0] ? "x" : "") + std::to_string(op1));
    if (op2 != NO_REGISTER && op2 != NO_IMMEDIATE) operands.push_back((isReg[1] ? "x" : "") + std::to_string(op2));
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

void Simulator::addRegisters(InstructionVariables& vars, std::vector<std::string>& printStatement, uint8_t opcode) {
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

void Simulator::printControlSignals(const ControlSignals& signals) {
    std::cout << "RegWrite = " << signals.RegWrite << "\n"
              << "MemRead = " << signals.MemRead << "\n"
              << "MemWrite = " << signals.MemWrite << "\n"
              << "MemToReg = " << signals.MemToReg << "\n"
              << "ALUSrc = " << signals.ALUSrc << "\n"
              << "Branch = " << signals.Branch << "\n"
              << "Jump = " << signals.Jump << "\n"
              << "JumpReg = " << signals.JumpReg << "\n"
              << "Zero = " << signals.Zero << "\n";
}

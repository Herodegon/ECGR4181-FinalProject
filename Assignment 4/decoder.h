#ifndef DECODER_H
#define DECODER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <bitset>
#include <limits>

// Define instruction formats
enum InstructionFormat {
    FORMAT_R,
    FORMAT_I,
    FORMAT_S,
    FORMAT_B,
    FORMAT_U,
    FORMAT_J
};

// Instruction map
InstructionMap Instructions = 
{
    {
        OPCODE_LOAD, 
        {
            {0b000, "lb"},
            {0b001, "lh"},
            {0b010, "lw"},
            {0b100, "lbu"},
            {0b101, "lhu"}
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
            {0b000, Funct7Map{{0b0000000, "add"}, {0b0100000, "sub"}, {0b0000001, "mul"}}},
            {0b001, Funct7Map{{0b0000000, "sll"}, {0b0000001, "mulh"}}},
            {0b010, Funct7Map{{0b0000000, "slt"}, {0b0000001, "mulhsu"}}},
            {0b011, Funct7Map{{0b0000000, "sltu"}, {0b0000001, "mulhu"}}},
            {0b100, Funct7Map{{0b0000000, "xor"}, {0b0000001, "div"}}},
            {0b101, Funct7Map{{0b0000000, "srl"}, {0b0100000, "sra"}, {0b0000001, "divu"}}},
            {0b110, Funct7Map{{0b0000000, "or"}, {0b0000001, "rem"}}},
            {0b111, Funct7Map{{0b0000000, "and"}, {0b0000001, "remu"}}}
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
    }
};

// Define control signals
struct ControlSignals
{
    bool RegWrite = false;
    bool MemRead = false;
    bool MemWrite = false;
    bool MemToReg = false;
    bool ALUSrc = false;
    bool Branch = false;
    bool Jump = false;
    bool JumpReg = false;
    bool Zero = false;
};

// Instruction variables
struct InstructionVariables
{
    int rs1 = NO_REGISTER;
    int rs2 = NO_REGISTER;
    int rd = NO_REGISTER;
    int funct3 = NO_FUNCT3;
    int funct7 = NO_FUNCT7;
    int immediate = NO_IMMEDIATE;
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

// Simulator class
class Simulator {
public:
    Simulator();

    // Decode instruction based on opcode
    void decodeInstruction(uint32_t instruction);

private:
    // Getters for instruction fields
    uint32_t getOpcode(uint32_t instruction);
    uint32_t getRS1(uint32_t instruction);
    uint32_t getRS2(uint32_t instruction);
    uint32_t getRD(uint32_t instruction);
    uint32_t getFunct3(uint32_t instruction);
    uint32_t getFunct7(uint32_t instruction);
    int32_t getImmediate(uint32_t instruction);

    // Operand and register management
    void printOperands(int op1, int op2, int op3, std::vector<std::string>& printStatement,
                       std::vector<bool> isReg, bool isImmediateLast = false);
    void addRegisters(InstructionVariables& vars, std::vector<std::string>& printStatement, uint8_t opcode);
    
    // Print control signals
    void printControlSignals(const ControlSignals& signals);
};

#endif // DECODER_H

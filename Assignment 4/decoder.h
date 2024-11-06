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

// Define Funct7Map and InstructionMap
using Funct7Map = std::unordered_map<int, std::string>;
using InstructionMap = std::unordered_map<uint32_t, std::unordered_map<int, std::variant<std::string, Funct7Map>>>;

// Define RISC-V opcodes
#define OPCODE_LOAD         0b0000011
#define OPCODE_LOAD_FP      0b0000111
#define OPCODE_I_TYPE       0b0010011
#define OPCODE_AUIPC        0b0010111
#define OPCODE_S_TYPE       0b0100011
#define OPCODE_S_TYPE_FP    0b0100111
#define OPCODE_R_TYPE       0b0110011
#define OPCODE_LUI          0b0110111
#define OPCODE_SB_TYPE      0b1100011
#define OPCODE_JALR         0b1100111
#define OPCODE_JAL          0b1101111

const int NO_IMMEDIATE = std::numeric_limits<int32_t>::max();
const int NO_REGISTER = std::numeric_limits<int32_t>::max();
const int NO_FUNCT3 = std::numeric_limits<int32_t>::max();
const int NO_FUNCT7 = std::numeric_limits<int32_t>::max();

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
extern InstructionMap InstructionMapping;

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
extern std::unordered_map<uint8_t, ControlSignals> ControlInstructions;

// Simulator class
class Decoder {
public:
    Decoder();

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

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <bitset>
#include <limits>

//====ASSIGNMENT 2====

const int STALL_INT = 10;       // Stall for integer instructions = 1 CPU cycle = 10 sim ticks
const int STALL_FLOAT = 50;     // Stall for floating point instructions = 5 CPU cycles = 50 sim ticks

struct Event
{
    std::string name;
    std::string stage;
    int cycle;
};

struct Instruction
{
    std::string name;
    std::vector<std::string> operands;
    std::string type;
    std::string stage;
    double data;
    std::map<std::string, int> cycle_entered;
    Instruction(std::string n, std::vector<std::string> ops, std::string t) : name(n), operands(ops), type(t), stage("Fetch"), data(0.0) {}
};

const std::vector<std::string> pipeline_stages = {"Fetch", "Decode", "Execute", "Store"};

class Simulator
{
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
        bool branch_pred = false;

    public:
        Simulator(int num_runs=0) : clock_cycle(0), clock_cycle_limit(num_runs), pc(0), halt(false), stall_count(0)
        {
            pipeline_registers["Fetch"] = nullptr;
            pipeline_registers["Decode"] = nullptr;
            pipeline_registers["Execute"] = nullptr;
            pipeline_registers["Store"] = nullptr;
            registers["x1"] = 160;
            registers["x2"] = 0;
            f_registers["f0"] = 0.0;
            f_registers["f2"] = 0.0;
            f_registers["f4"] = 0.0;
            load_instructions();
        }

        void fetch()
        {
            if (stall_count > 0)
            {
                std::cout << "Cycle " << clock_cycle << ": Fetch stage is stalled." << std::endl;
                return;
            }

            if (pc < instructions.size())
            {
                Instruction* instr = instructions[pc];
                pc++;
                instr->stage = "Fetch";
                instr->cycle_entered["Fetch"] = clock_cycle;
                pipeline_registers["Fetch"] = instr;
                event_list.push_back({instr->name, "Fetch", clock_cycle});
                std::cout << "Cycle " << clock_cycle << ": Fetching instruction " << instr->name << std::endl;
            }
            else
            {
                pipeline_registers["Fetch"] = nullptr;      // If no more instructions, set fetch stage to null
            }
            pc = pc % instructions.size();
        }

        void decode()
        {
            if (stall_count > 0)
            {
                std::cout << "Cycle " << clock_cycle << ": Decode stage is stalled." << std::endl;
                return;
            }

            Instruction* instr = pipeline_registers["Fetch"];
            if (instr)
            {
                clean_event_list(instr);
                instr->stage = "Decode";
                instr->cycle_entered["Decode"] = clock_cycle;
                pipeline_registers["Decode"] = instr;
                event_list.push_back({instr->name, "Decode", clock_cycle});
                std::cout << "Cycle " << clock_cycle << ": Decoding instruction " << instr->name << std::endl;
            }
            else
            {
                pipeline_registers["Decode"] = nullptr;     // If no instruction in the fetch stage, set decode stage to null
            }
        }

        void execute()
        {
            if (stall_count > 0)
            {
                std::cout << "Cycle " << clock_cycle << ": Execute stage is stalled." << std::endl;
                return;
            }

            Instruction* instr = pipeline_registers["Decode"];
            if (instr)
            {
                clean_event_list(instr);
                instr->stage = "Execute";
                instr->cycle_entered["Execute"] = clock_cycle;
                pipeline_registers["Execute"] = instr;
                event_list.push_back({instr->name, "Execute", clock_cycle});
                std::cout << "Cycle " << clock_cycle << ": Executing instruction " << instr->name << std::endl;
                pipeline_registers["Decode"] = nullptr;
            }
            else
            {
                pipeline_registers["Execute"] = nullptr;   // If no instruction in the decode stage, set execute stage to null
            }
        }

        void store()
        {
            Instruction* instr = pipeline_registers["Execute"];
            if (instr)
            {
                clean_event_list(instr);
                if (instr->type == "F" && instr->name != "fsd")
                {
                    if (instr->name == "fadd.d" || instr->name == "fsub.d" || instr->name == "fmul.d" || instr->name == "fdiv.d")
                    {
                        stall_count++;
                    }
                    stall_count++;
                }

                instr->stage = "Store";
                instr->cycle_entered["Store"] = clock_cycle;
                pipeline_registers["Store"] = instr;
                event_list.push_back({instr->name, "Store", clock_cycle});
                std::cout << "Cycle " << clock_cycle << ": Storing instruction " << instr->name << std::endl;
                execute_instruction(instr);
                pipeline_registers["Execute"] = nullptr;
            }
            else
            {
                pipeline_registers["Store"] = nullptr;    // If no instruction in the execute stage, set store stage to null
            }
        }

        void load_instructions(std::string prog_call = "")
        {
            instructions.push_back(new Instruction("fld", {"f0", "0(x1)"}, "F"));           // fld f0, 0(x1)
            instructions.push_back(new Instruction("fadd.d", {"f4", "f0", "f2"}, "F"));     // fadd.d f4, f0, f2
            instructions.push_back(new Instruction("fsd", {"f4", "0(x1)"}, "F"));           // fsd f4, 0(x1)
            instructions.push_back(new Instruction("addi", {"x1", "x1", "-8"}, "I"));       // addi x1, x1, -8
            instructions.push_back(new Instruction("bne", {"x1", "x2", "Loop"}, "B"));      // bne x1, x2, Loop
        }

        void clean_event_list(Instruction* instr)
        {
            for (auto it = event_list.begin(); it != event_list.end();)
            {
                if (it->name == instr->name && it->stage == instr->stage)
                {
                    it = event_list.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void execute_instruction(Instruction* instr)
        {
            std::string name = instr->name;
            if (name == "fld")
            {
                int offset = std::stoi(instr->operands[1].substr(0, instr->operands[1].find('(')));
                std::string reg = instr->operands[1].substr(instr->operands[1].find('(') + 1);
                reg = reg.substr(0, reg.find(')'));
                int addr = registers[reg] + offset;
                f_registers[instr->operands[0]] = memory[addr];
                instr->data = f_registers[instr->operands[0]];
            }
            else if (name == "fadd.d")
            {
                f_registers[instr->operands[0]] = f_registers[instr->operands[1]] + f_registers[instr->operands[2]];
                instr->data = f_registers[instr->operands[0]];
            }
            else if (name == "fsd")
            {
                int offset = std::stoi(instr->operands[1].substr(0, instr->operands[1].find('(')));
                std::string reg = instr->operands[1].substr(instr->operands[1].find('(') + 1);
                reg = reg.substr(0, reg.find(')'));
                int addr = registers[reg] + offset;
                memory[addr] = f_registers[instr->operands[0]];
                instr->data = f_registers[instr->operands[0]];
            }
            else if (name == "addi")
            {
                registers[instr->operands[0]] = registers[instr->operands[1]] + std::stoi(instr->operands[2]);
                instr->data = registers[instr->operands[0]];
            }
            else if (name == "bne")
            {
                if(registers[instr->operands[0]] != registers[instr->operands[1]])
                {

                }
                else
                {
                    halt = true;
                }
            }
            else
            {
                // Handle other instructions if necessary
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

int main()
{
    int limit = 0;
    Simulator sim(limit);
    sim.run();
    return 0;
}

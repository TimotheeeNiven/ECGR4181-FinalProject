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

struct Event {
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

// BTB Entry structure
struct BTBEntry {
    int address;    // Address of the branch instruction
    int target;     // Target address to branch to
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
    std::unordered_map<int, BTBEntry> btb;                   // Branch Target Buffer

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


    void load_instructions(std::string prog_call = "") {
        // Load instructions corresponding to the assembly code
        instructions.push_back(new Instruction("la", {"x10", "ARRAY_A_ADDR"}, "I"));   // la x10, ARRAY_A_ADDR
        instructions.push_back(new Instruction("la", {"x11", "ARRAY_B_ADDR"}, "I"));   // la x11, ARRAY_B_ADDR
        instructions.push_back(new Instruction("la", {"x12", "ARRAY_C_ADDR"}, "I"));   // la x12, ARRAY_C_ADDR
        instructions.push_back(new Instruction("li", {"x13", "0"}, "I"));               // li x13, 0
        instructions.push_back(new Instruction("li", {"x14", "256"}, "I"));             // li x14, 256
        instructions.push_back(new Instruction("flw", {"f0", "0(x10)"}, "F"));          // flw f0, 0(x10)
        instructions.push_back(new Instruction("flw", {"f1", "0(x11)"}, "F"));          // flw f1, 0(x11)
        instructions.push_back(new Instruction("fadd.s", {"f2", "f0", "f1"}, "F"));     // fadd.s f2, f0, f1
        instructions.push_back(new Instruction("fsw", {"f2", "0(x12)"}, "F"));          // fsw f2, 0(x12)
        instructions.push_back(new Instruction("bge", {"x13", "x14", "end_loop"}, "B")); // bge x13, x14, end_loop
        instructions.push_back(new Instruction("j", {"loop"}, "B"));                    // jump back to loop
        instructions.push_back(new Instruction("end_loop", {}, "B"));                   // end_loop label
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
        std::string name = instr->name;
        if (name == "la") {
            // Load address into the register (here we assume the address is hardcoded)
            if (instr->operands[0] == "x10") {
                registers["x10"] = 0x0400; // Address for ARRAY_A
            } else if (instr->operands[0] == "x11") {
                registers["x11"] = 0x0800; // Address for ARRAY_B
            } else if (instr->operands[0] == "x12") {
                registers["x12"] = 0x0C00; // Address for ARRAY_C
            }
        } else if (name == "li") {
            // Load immediate value into the register
            registers[instr->operands[0]] = std::stoi(instr->operands[1]);
        } else if (name == "flw") {
            // Load floating point word
            f_registers[instr->operands[0]] = memory[registers[instr->operands[1]]];
        } else if (name == "fadd.s") {
            // Floating point addition
            f_registers[instr->operands[0]] = f_registers[instr->operands[1]] + f_registers[instr->operands[2]];
        } else if (name == "fsw") {
            // Store floating point word
            memory[registers[instr->operands[1]]] = f_registers[instr->operands[0]];
        } else if (name == "bge") {
            // Branch if greater than or equal
            if (registers[instr->operands[0]] >= registers[instr->operands[1]]) {
                pc = instructions.size(); // Set PC to the end to branch
            }
        } else if (name == "j") {
            // Unconditional jump
            pc = instructions.size(); // Set PC to the end to jump
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
    int limit = 20;
    Simulator sim(limit);
    sim.run();
    return 0;
}

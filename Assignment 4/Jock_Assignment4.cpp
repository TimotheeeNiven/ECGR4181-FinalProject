#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <bitset>
#include <limits>

//====ASSIGNMENT 3====

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
const int NO_REGISTER = std::numeric_limits<int32_t>::max();;
const int NO_FUNCT3 = std::numeric_limits<int32_t>::max();;
const int NO_FUNCT7 = std::numeric_limits<int32_t>::max();;

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

class Simulator 
{
public:
    Simulator() {};

    // Decode instruction based on opcode
    void decodeInstruction(uint32_t instruction)
    {
        uint8_t opcode = getOpcode(instruction);
        ControlSignals signals = ControlInstructions[opcode];
        InstructionVariables vars;
        std::vector<std::string> printStatement;

        InstructionFormat format;

        switch(opcode)
        {
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

        if(instructionType.index() == 0)
        {
            std::string tempStr = std::get<std::string>(instructionType);
            printStatement.push_back(tempStr);
        }
        else
        {
            Funct7Map funct7Map = std::get<Funct7Map>(instructionType);
            std::string tempStr = funct7Map[vars.funct7];
            printStatement.push_back(tempStr);
        }

        addRegisters(vars, printStatement, opcode);
        printControlSignals(signals);

        // Output the assembled instruction print statement
        for (size_t i = 0; i < printStatement.size(); ++i) {
            std::cout << printStatement[i];
            if (i != printStatement.size() - 1) {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
        
    }

private:
    // Getters for instruction fields
    uint32_t getOpcode(uint32_t instruction)
    {
        return instruction & 0x7F;
    }

    uint32_t getRS1(uint32_t instruction)
    {
        return (instruction >> 15) & 0x1F;
    }

    uint32_t getRS2(uint32_t instruction)
    {
        return (instruction >> 20) & 0x1F;
    }

    uint32_t getRD(uint32_t instruction)
    {
        return (instruction >> 7) & 0x1F;
    }

    uint32_t getFunct3(uint32_t instruction)
    {
        return (instruction >> 12) & 0x7;
    }

    uint32_t getFunct7(uint32_t instruction)
    {
        return (instruction >> 25) & 0x7F;
    }

    int32_t getImmediate(uint32_t instruction) {
        uint32_t opcode = getOpcode(instruction);
        int32_t imm = 0;

        switch(opcode) {
            case OPCODE_LOAD:
            case OPCODE_I_TYPE:
            case OPCODE_JALR:
                imm = (instruction >> 20) & 0xFFF;
                // Sign-extend 12-bit immediate
                if (imm & 0x800)
                    imm |= 0xFFFFF000;
                
                break;
            case OPCODE_S_TYPE:
                imm = ((instruction >> 25) & 0x7F) << 5 |
                      (instruction >> 7) & 0x1F;
                // Sign-extend 12-bit immediate
                if (imm & 0x800)
                    imm |= 0xFFFFF000;
                
                break;
            case OPCODE_SB_TYPE:
                imm = ((instruction >> 31) & 0x1) << 12 |
                      ((instruction >> 7) & 0x1) << 11 |
                      ((instruction >> 25) & 0x3F) << 5 |
                      ((instruction >> 8) & 0xF) << 1;
                // Sign-extend 13-bit immediate
                if (imm & 0x1000)
                    imm |= 0xFFFFE000;
                
                break;
            case OPCODE_AUIPC:
            case OPCODE_LUI:
                imm = instruction >> 12;
                // Sign-extend 20-bit immediate
                if (imm & 0x80000)
                    imm |= 0xFFF00000;
                break;
            case OPCODE_JAL:
                imm = ((instruction >> 31) & 0x1) << 20 |
                      ((instruction >> 21) & 0x3FF) << 1 |
                      ((instruction >> 20) & 0x1) << 11 |
                      ((instruction >> 12) & 0xFF) << 12;
                // Sign-extend 21-bit immediate
                if (imm & 0x100000)
                    imm |= 0xFFE00000;
                
                break;
            default:
                imm = 0;
                break;
        }

        return imm;
    }

    void printOperands(int op1, int op2, int op3, std::vector<std::string>& printStatement,
                    std::vector<bool> isReg, bool isImmediateLast = false) {
        std::vector<std::string> operands;

        if (op1 != NO_REGISTER) {
            operands.push_back((isReg[0] ? "x" : "") + std::to_string(op1));
        }

        if (op2 != NO_REGISTER && op2 != NO_IMMEDIATE) {
            operands.push_back((isReg[1] ? "x" : "") + std::to_string(op2));
        }

        if (op3 != NO_REGISTER && op3 != NO_IMMEDIATE) {
            std::string operand;
            if (isImmediateLast) {
                operand = std::to_string(op3);
            } else {
                operand = (isReg[2] ? "x" : "") + std::to_string(op3);
            }
            operands.push_back(operand);
        }

        // Add commas between operands
        for (size_t i = 0; i < operands.size(); ++i) {
            printStatement.push_back(operands[i]);
            if (i != operands.size() - 1) {
                printStatement.back() += ",";
            }
        }
    }

    void addRegisters(InstructionVariables& vars, std::vector<std::string>& printStatement, uint8_t opcode) {
        switch (opcode) {
            case OPCODE_R_TYPE:
                // R-type: rd, rs1, rs2
                printOperands(vars.rd, vars.rs1, vars.rs2, printStatement, {true, true, true});
                break;
            case OPCODE_I_TYPE:
                // I-type: rd, rs1, imm
                printOperands(vars.rd, vars.rs1, vars.immediate, printStatement, {true, true, false}, true);
                break;
            case OPCODE_LOAD:
            case OPCODE_LOAD_FP:
                // Load instructions: rd, offset(rs1)
                if (vars.rd != NO_REGISTER) {
                    printStatement.push_back("x" + std::to_string(vars.rd) + ",");
                }
                if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER) {
                    printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
                }
                break;
            case OPCODE_S_TYPE:
            case OPCODE_S_TYPE_FP:
                // S-type: rs2, offset(rs1)
                if (vars.rs2 != NO_REGISTER) {
                    printStatement.push_back("x" + std::to_string(vars.rs2) + ",");
                }
                if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER) {
                    printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
                }
                break;
            case OPCODE_SB_TYPE:
                // B-type: rs1, rs2, imm
                printOperands(vars.rs1, vars.rs2, vars.immediate, printStatement, {true, true, false}, true);
                break;
            case OPCODE_LUI:
            case OPCODE_AUIPC:
                // U-type: rd, imm
                printOperands(vars.rd, vars.immediate, NO_IMMEDIATE, printStatement, {true, false, false}, true);
                break;
            case OPCODE_JAL:
                // J-type: rd, imm
                printOperands(vars.rd, vars.immediate, NO_IMMEDIATE, printStatement, {true, false, false}, true);
                break;
            case OPCODE_JALR:
                // JALR instruction: rd, offset(rs1)
                if (vars.rd != NO_REGISTER) {
                    printStatement.push_back("x" + std::to_string(vars.rd) + ",");
                }
                if (vars.immediate != NO_IMMEDIATE && vars.rs1 != NO_REGISTER) {
                    printStatement.push_back(std::to_string(vars.immediate) + "(x" + std::to_string(vars.rs1) + ")");
                }
                break;
            default:
                // Handle unknown formats or instructions
                break;
        }
    }

    void printControlSignals(const ControlSignals& signals)
    {
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

};

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

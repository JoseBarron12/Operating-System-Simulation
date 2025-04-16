#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "memory.h" 
#include "program.h"
#include "Opcode.h"

Program::Program(const std::string& filename)
{
    loadProgram(filename);
}

void Program::loadProgram(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Program file unable to open";
        return;
    }
    std::string instruction;

    while(std::getline(file,instruction))
    {
        convertInstruction(instruction);
    }
}

void Program::convertInstruction(const std::string& line)
{
    if (line.empty()) return; 
    
    std::istringstream iss(line);
    std::string opcode;
    std::string param1, param2;

    iss >> opcode >> param1 >> param2;

    uint32_t binaryopcode = static_cast<uint32_t>(convertString(opcode));
    uint32_t arg1 = 0, arg2 = 0;
    
    if (!param1.empty()) 
    {
        if (param1[0] == 'r')
            arg1 = std::stoi(param1.substr(1));
        else if (param1[0] == '$')
            arg1 = std::stoi(param1.substr(1), nullptr, 0);
        else
            arg1 = std::stoi(param1);
    }

    if (!param2.empty()) 
    {
        if (param2[0] == 'r')
            arg2 = std::stoi(param2.substr(1));
        else if (param2[0] == '$')
            arg2 = std::stoi(param2.substr(1), nullptr, 0);
        else
            arg2 = std::stoi(param2);
    }

    instructions.push_back(binaryopcode);
    instructions.push_back(arg1);
    instructions.push_back(arg2);
}

void Program::loadIntoMemory(memory& mem, uint32_t start, uint32_t pid)
{
    size_t addr = start;
    size_t addr_pt = start;
    size_t start_addr = 0;
    
    for (size_t i = 0; i < mem.getpages(); i++)
    {
        if ( addr_pt == 256)
        {
            start_addr = 512;
        }
        else if  (addr_pt == 512)
        {
            start_addr = 256;
        }
        else
        {
            start_addr = addr_pt;
        }
        if (!mem.pageExists(start_addr))  
        {
            mem.mapPage(start_addr,i,pid);
        }
        addr_pt += PAGE_SIZE;
    }

    for (size_t i = 0; i < instructions.size(); i += 3) 
    {
        uint32_t physical_addr = mem.getaddress(addr, pid); 

        mem.set8(physical_addr, instructions[i]);
        mem.set32(physical_addr + 1, instructions[i+1]); 
        mem.set32(physical_addr + 5, instructions[i+2]); 

        addr += 9; 
    }
}


Opcode Program::convertString(const std::string instruction)
{
    static const std::unordered_map<std::string, Opcode> opcodeMap = 
    {
        {"incr", Incr}, {"addi", Addi}, {"addr", Addr}, {"pushr", Pushr},
        {"pushi", Pushi}, {"movi", Movi}, {"movr", Movr}, {"movmr", Movmr},
        {"movrm", Movrm}, {"movmm", Movmm}, {"printr", Printr}, {"printm", Printm},
        {"printcr", Printcr}, {"printcm", Printcm}, {"jmp", Jmp}, {"jmpi", Jmpi},
        {"jmpa", Jmpa}, {"cmpi", Cmpi}, {"cmpr", Cmpr}, {"jlt", Jlt},
        {"jlti", Jlti}, {"jlta", Jlta}, {"jgt", Jgt}, {"jgti", Jgti},
        {"jgta", Jgta}, {"je", Je}, {"jei", Jei}, {"jea", Jea},
        {"call", Call}, {"callm", Callm}, {"ret", Ret}, {"exit", Exit},
        {"popr", Popr}, {"popm", Popm}, {"sleep", Sleep}, {"input", Input},
        {"inputc", Inputc}, {"setPriority", SetPriority}, {"setPriorityI", SetPriorityI},
        {"MapSharedMem", MapSharedMem}, {"AcquireLock", AcquireLock}, {"AcquireLockI", AcquireLockI},
        {"ReleaseLock", ReleaseLock}, {"ReleaseLockI", ReleaseLockI}, {"SignalEvent", SignalEvent},
        {"SignalEventI", SignalEventI}, {"WaitEvent", WaitEvent}, {"WaitEventI", WaitEventI},
        {"Alloc", Alloc}, {"FreeMemory", FreeMemory}, {"TerminateProcess", TerminateProcess}
    };

    auto it = opcodeMap.find(instruction);
    if (it != opcodeMap.end()) 
    {
        return it->second;  
    } 
    else 
    {
        std::cerr << "Error: Invalid Opcode: " << instruction;
        return Opcode::Exit;
    }
}

uint32_t Program::getCodeSize() const 
{
    return (instructions.size() / 3) * 9;
}
// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "memory.h" 
#include "program.h"
#include "Opcode.h"

/**
 * Constructs a Program object and loads the program from the provided file.
 * @param filename Path to the assembly-like program file.
 */
Program::Program(const std::string& filename)
{
    loadProgram(filename);
}

/**
 * Reads instructions from a text file and converts them to binary format.
 * Removes comments and whitespace, then delegates conversion to convertInstruction().
 * @param filename Name of the file containing the instructions.
 */
void Program::loadProgram(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Program file unable to open";
        return;
    }
    std::string instruction;

    while (std::getline(file, instruction))
    {
        size_t semicolon = instruction.find(';');
        size_t hash = instruction.find('#');
        
        size_t comment = std::min(semicolon, hash);
        
        if (semicolon == std::string::npos) 
        {
            comment = hash;
        }
        else if (hash != std::string::npos) 
        {
            comment = std::min(semicolon, hash);
        }
        

        if (comment != std::string::npos)
        {
            instruction = instruction.substr(0, comment); // Remove comment part
        }
            
        instruction.erase(0, instruction.find_first_not_of(" \t\r\n")); 
        instruction.erase(instruction.find_last_not_of(" \t\r\n") + 1); 

        convertInstruction(instruction);
            
    }

}

/**
 * Parses a single line of instruction and adds the binary form to the instruction list.
 * With the assumption that each instruction is 9 bytes, and both arguments are always read into memory.
 * @param line A cleaned and trimmed line of code with opcode and optional parameters.
 */
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

/**
 * Loads the program's instructions into memory starting at a virtual address.
 * Handles page mapping as needed and writes binary instructions sequentially.
 * @param mem Memory reference to write to.
 * @param start Starting virtual address.
 * @param pid Process ID performing the write.
 */
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

/**
 * Converts a string mnemonic into an Opcode enum value.
 * Case-insensitive and uses a lookup table for mapping.
 * @param instruction The string mnemonic to convert.
 * @return The corresponding Opcode enum value, or Exit if invalid.
 */
Opcode Program::convertString(std::string instruction)
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
        {"inputc", Inputc}, {"setpriority", SetPriority}, {"setpriorityi", SetPriorityI},
        {"mapsharedmem", MapSharedMem}, {"acquirelock", AcquireLock}, {"acquirelocki", AcquireLockI},
        {"releaselock", ReleaseLock}, {"releaselocki", ReleaseLockI}, {"signalevent", SignalEvent},
        {"signaleventi", SignalEventI}, {"waitevent", WaitEvent}, {"waiteventi", WaitEventI},
        {"alloc", Alloc}, {"freememory", FreeMemory}, {"terminateprocess", TerminateProcess}
    };

    for (auto& c : instruction)
    {
        c = std::tolower(static_cast<unsigned char>(c));
    }

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

/**
 * Calculates the total code size in bytes.
 * Each instruction takes 9 bytes: 1 (opcode) + 4 (arg1) + 4 (arg2).
 * @return Size of the instruction section of the program in bytes.
 */
uint32_t Program::getCodeSize() const 
{
    return (instructions.size() / 3) * 9;
}
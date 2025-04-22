#ifndef PROGRAM_H
#define PROGRAM_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "memory.h" 
#include "Opcode.h"

class Program 
{
private:
    std::vector<uint32_t> instructions;

public:
    Program(const std::string& filename);
    void loadProgram(const std::string& filename);
    void convertInstruction(const std::string& line);
    void loadIntoMemory(memory& mem,uint32_t start, uint32_t pid);
    Opcode convertString(std::string instruction);
    uint32_t getCodeSize() const;
    std::vector<uint32_t> getInstructions() const { return instructions; }
};

#endif
// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef PROGRAM_H
#define PROGRAM_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "memory.h" 
#include "Opcode.h"


/**
 * @class Program
 * @brief The Program class is responsible for loading and converting a
 *        program file into binary instructions, and for loading them into memory.
 */
class Program 
{
private:
    std::vector<uint32_t> instructions; // Stores the program's instructions in binary form
public:
    /// @brief Program constructor
    Program(const std::string& filename);
    
    /// @brief Methods that handle loading program into memory correctly
    void loadProgram(const std::string& filename);
    void convertInstruction(const std::string& line);
    void loadIntoMemory(memory& mem,uint32_t start, uint32_t pid);
    
    /// @brief Helper methods
    Opcode convertString(std::string instruction);
    uint32_t getCodeSize() const;
    std::vector<uint32_t> getInstructions() const { return instructions; }
};

#endif
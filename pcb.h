// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef PCB_H_INC
#define PCB_H_INC

#include <vector>
#include <cstdint>
#include "memory.h"
#include "program.h"

/**
 * Represents a single heap memory block used by a process.
 */
struct HeapPage {
    uint32_t addr;  // Starting address of the heap block
    uint32_t size;  // Size of the heap block
    bool used;      // Flag indicating whether the block is in use
};

/**
 * @class PCB
 * @brief Process Control Block  structure encapsulates all information
 * needed to manage and schedule a process in the operating system.
 */
class PCB {
public:
    uint32_t processId;          // Unique ID of the process
    uint32_t codeSize;           // Size of the code segment
    uint32_t stackSize;          // Size of the stack segment
    uint32_t dataSize;           // Size of the data segment
    uint32_t heapStart;          // Start of heap segment
    uint32_t heapEnd;            // End of heap segment
    uint32_t heapSize;           // Size of the heap segment
    uint32_t processMemorySize;  // Total size of process memory
    
    
    uint32_t registers[15];     // CPU registers
    bool signFlag;              // Sign flag
    bool zeroFlag;              // Zero flag 
    uint32_t priority;          // Current process priority
    uint32_t originalPriority;  // Initial priority
    uint32_t timeQuantum;       // Time slice allocated for the process
    uint32_t clockCycles;       // Total number of executed cycles for cpu
    uint32_t totalClockCycles;  // Total number of executed cycles
    uint32_t sleepCounter;      // Remaining sleep time 
    uint32_t contextSwitches;   // Number of times this process was switched in or out
    
    
    std::vector<uint32_t> workingSetPages;  // Virtual pages used by the process
    Program* program;                       // Pointer to loaded Program object
    std::string state;                      // Process state (Ready, Running, Sleeping, or Waiting)  
    std::vector<HeapPage> heapAllocations;  // Allocated heap memory blocks
    
    
    bool isLoaded = false;        // Loaded Flag
    bool waitingForLock = false;  // Lock Flag
    bool isBlocked = false;       // Blocked Flag
    
    /// @brief Process Control Block constructor
    PCB(uint32_t id, uint32_t codeSize, uint32_t stackSize, uint32_t dataSize, uint32_t priority, Program* prog, uint32_t heapSize);
    
    /// @brief Methods that handle cpu states
    void saveState(uint32_t* cpuRegisters, bool signFlag, bool zeroFlag);
    void restoreState(uint32_t* cpuRegisters, bool& signFlag, bool& zeroFlag);
    
    /// @brief Helper Methods
    void incrementClockCycle() { clockCycles++; }
    uint32_t getPid() { return processId; }
    void settimeQuantum( uint32_t time) { timeQuantum = time;}

};

#endif 


// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef OPERATING_SYSTEM_H
#define OPERATING_SYSTEM_H

#include "cpu.h"
#include "memory.h"
#include "program.h"
#include "pcb.h"
#include "ProcessScheduler.h"
#include <queue>

/**
 * @class OperatingSystem
 * @brief Simulates an operating system that manages memory, CPU, processes, and scheduling.
 */
class OperatingSystem 
{
private:       
        
    ProcessScheduler scheduler;      // Manages the ready queue and process selection 
    std::vector<PCB*> processList;   // List of all loaded processes
    std::string programFile;         // Program file name
    uint32_t currentMemAddress = 0;  // Tracks memory allocation pointer

public:
    /// @brief System Constructor
    OperatingSystem(uint32_t memorySize, const std::string& programFile);
    
    CPU cpu;    // Operation systems CPU
    memory mem; // Operating systems memory
    
    /// @brief Methods used to load or terminate process
    void loadProcess(uint32_t id, const std::string& programFile, uint32_t stackSize, uint32_t dataSize, uint32_t priority, uint32_t HeapSize);
    void loadIdleProcess(const std::string& file);
    bool terminateProcessByPid(uint32_t pid);
    
    /// @brief Main method that runs operating system
    void start();  
    
    /// @brief Helper Methods
    bool hasSleepingOrWaitingProcesses();
    bool hasAllSleepingOrWaitingProcesses();
    void unblockProcesses();
    
};

#endif

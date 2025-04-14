#ifndef OPERATING_SYSTEM_H
#define OPERATING_SYSTEM_H

#include "cpu.h"
#include "memory.h"
#include "program.h"
#include "pcb.h"
#include "ProcessScheduler.h"
#include <queue>

class OperatingSystem 
{
private:
    CPU cpu;           
    memory mem;    
    ProcessScheduler scheduler;
    std::vector<PCB*> processList;
    std::string programFile;
    uint32_t currentMemAddress = 0;

public:
    OperatingSystem(uint32_t memorySize, const std::string& programFile);
    void loadProcess(uint32_t id, const std::string& programFile, uint32_t stackSize, uint32_t dataSize, uint32_t priority, uint32_t HeapSize);
    void start();  
};

#endif

#ifndef PCB_H_INC
#define PCB_H_INC

#include <vector>
#include <cstdint>
#include "memory.h"
#include "program.h"

struct HeapPage {
    uint32_t addr;
    uint32_t size;
    bool used;
};

class PCB {
public:
    uint32_t processId;
    uint32_t codeSize;
    uint32_t stackSize;
    uint32_t dataSize;
    uint32_t heapStart;
    uint32_t heapEnd;
    uint32_t heapSize;
    uint32_t processMemorySize;
    uint32_t registers[15];
    bool signFlag;
    bool zeroFlag;
    uint32_t priority;
    uint32_t timeQuantum;
    uint32_t clockCycles;
    uint32_t sleepCounter;
    uint32_t contextSwitches;
    std::vector<uint32_t> workingSetPages;
    Program* program;
    std::string state;
    std::vector<HeapPage> heapAllocations;
    bool isLoaded = false;
    
    PCB(uint32_t id, uint32_t codeSize, uint32_t stackSize, uint32_t dataSize, uint32_t priority, Program* prog, uint32_t heapSize);
    void saveState(uint32_t* cpuRegisters, bool signFlag, bool zeroFlag);
    void restoreState(uint32_t* cpuRegisters, bool& signFlag, bool& zeroFlag);
    void incrementClockCycle() { clockCycles++; }
    uint32_t getPid() { return processId; }
    void settimeQuantum( uint32_t time) { timeQuantum = time;}

};

#endif 


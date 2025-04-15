#include "pcb.h"

PCB::PCB(uint32_t id, uint32_t codeSize, uint32_t stackSize, uint32_t dataSize, uint32_t priority, Program* prog, uint32_t heapSiz)
    : processId(id), codeSize(codeSize), stackSize(stackSize), dataSize(dataSize), priority(priority), program(prog) {
    
    heapStart = codeSize + dataSize;
    heapEnd = heapStart + heapSiz; 
    heapSize = heapSiz;
    processMemorySize = codeSize + stackSize + dataSize + heapSize;
   
    signFlag = false;
    zeroFlag = false;
    timeQuantum = 10;
    clockCycles = 0;
    sleepCounter = 0;
    contextSwitches = 0;
    originalPriority = priority;

    registers[11] = id;
    registers[12] = codeSize;
    registers[13] = codeSize + dataSize + heapSize + stackSize - 1;

    std::cout << "[MEMORY MAP] Process " << processId << " layout:\n";
    std::cout << "  Code:   0x000 - 0x" << std::hex << codeSize - 1 << "\n";
    std::cout << "  Data:   0x" << codeSize << " - 0x" << codeSize + dataSize - 1 << "\n";
    std::cout << "  Heap:   0x" << heapStart << " - 0x" << heapEnd - 1 << "\n";
    std::cout << "  Stack:  0x" << heapEnd << " - 0x" << heapEnd + stackSize - 1 << "\n";
    std::cout << "  SP(r13): 0x" << registers[13] << "\n" << std::dec;
        
}


void PCB::saveState(uint32_t* cpuRegisters, bool signFlagState, bool zeroFlagState) {
    for (int i = 0; i < 15; i++) {
        registers[i] = cpuRegisters[i];
    }
    signFlag = signFlagState;
    zeroFlag = zeroFlagState;
}

void PCB::restoreState(uint32_t* cpuRegisters, bool& signFlagState, bool& zeroFlagState) {
    for (int i = 0; i < 15; i++) {
        cpuRegisters[i] = registers[i];
    }
    signFlagState = signFlag;
    zeroFlagState = zeroFlag;
}
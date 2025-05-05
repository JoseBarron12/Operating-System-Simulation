// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "pcb.h"

/**
 * Constructor for PCB (Process Control Block).
 * Initializes memory layout, priority, and register values for a process.
 *
 * @param id         Process ID
 * @param codeSize   Size of the code segment
 * @param stackSize  Size of the stack segment
 * @param dataSize   Size of the data segment
 * @param priority   Initial priority of the process
 * @param prog       Pointer to associated Program object
 * @param heapSiz    Size of the heap segment
 */
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

    // Debugging memory layout printout
    std::cout << "[MEMORY MAP] Process " << processId << " layout:\n";
    std::cout << "  Code:   0x000 - 0x" << std::hex << codeSize - 1 << "\n";
    std::cout << "  Data:   0x" << codeSize << " - 0x" << codeSize + dataSize - 1 << "\n";
    std::cout << "  Heap:   0x" << heapStart << " - 0x" << heapEnd - 1 << "\n";
    std::cout << "  Stack:  0x" << heapEnd << " - 0x" << heapEnd + stackSize - 1 << "\n";
    std::cout << "  SP(r13): 0x" << registers[13] << "\n" << std::dec;
        
}

/**
 * Saves the CPU state (registers and flags) into the PCB.
 *
 * @param cpuRegisters   Pointer to CPU register array
 * @param signFlagState  Sign flag state to store
 * @param zeroFlagState  Zero flag state to store
 */
void PCB::saveState(uint32_t* cpuRegisters, bool signFlagState, bool zeroFlagState) {
    for (int i = 0; i < 15; i++) {
        registers[i] = cpuRegisters[i];
    }
    signFlag = signFlagState;
    zeroFlag = zeroFlagState;
}

/**
 * Restores the CPU state from the PCB into CPU registers and flags.
 *
 * @param cpuRegisters      Pointer to CPU register array to be restored
 * @param signFlagState     Sign flag reference to set
 * @param zeroFlagState     Zero flag reference to set
 */
void PCB::restoreState(uint32_t* cpuRegisters, bool& signFlagState, bool& zeroFlagState) {
    for (int i = 0; i < 15; i++) {
        cpuRegisters[i] = registers[i];
    }
    signFlagState = signFlag;
    zeroFlagState = zeroFlag;
}
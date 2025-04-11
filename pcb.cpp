#include "pcb.h"

PCB::PCB(uint32_t id, uint32_t codeSize, uint32_t stackSize, uint32_t dataSize, uint32_t priority, Program* prog, uint32_t heapSiz)
    : processId(id), codeSize(codeSize), stackSize(stackSize), dataSize(dataSize), priority(priority), program(prog) {
    heapStart = codeSize + dataSize;
    heapEnd = heapStart + heapSiz; 
    heapSize = heapSiz;
    processMemorySize = codeSize + stackSize + dataSize + heapSize;
    signFlag = false;
    zeroFlag = false;
    timeQuantum = 5;
    clockCycles = 0;
    sleepCounter = 0;
    contextSwitches = 0;
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
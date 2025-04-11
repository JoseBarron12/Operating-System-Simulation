#include "OperatingSystem.h"
#include <iostream>
#include <algorithm>

OperatingSystem::OperatingSystem(uint32_t memorySize, const std::string& programFile)
    : mem(memorySize), cpu(mem, scheduler), programFile(programFile) {
    currentMemAddress = 0;

    loadProcess(1, programFile, 256, 256, 1, 1024);
}

const std::vector<uint32_t>& OperatingSystem::getSharedPages() const {
    return sharedPages;
}


void OperatingSystem::loadProcess(uint32_t id, const std::string& programFile, uint32_t stackSize, uint32_t dataSize, uint32_t priority, uint32_t heapSize) 
{
    Program* newProgram = new Program(programFile);
    newProgram->loadProgram(programFile);

    uint32_t codeSize = newProgram->getCodeSize();
    uint32_t totalMemorySize = codeSize + stackSize + dataSize + heapSize;

    currentMemAddress = (currentMemAddress + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t programStartAddress = currentMemAddress;

    uint32_t alignedSize = (totalMemorySize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint32_t pagesNeeded = alignedSize / PAGE_SIZE;
    

    currentMemAddress += alignedSize;

    PCB* newProcess = new PCB(id, codeSize, stackSize, dataSize, priority, newProgram, heapSize);

    newProcess->registers[11] = programStartAddress;

    std::cout << "[MEMORY ALLOCATION] Process " << id << " assigned memory at "
              << programStartAddress << " - " << (programStartAddress + alignedSize) << std::endl;
    std::cout << "[DEBUG] Set PCB IP to: " << std::hex << newProcess->registers[11] << std::endl;

    std::cout << std::hex << "[MEMORY ALLOCATION] Process " << id 
          << " assigned memory at 0x" << programStartAddress 
          << " - 0x" << (programStartAddress + alignedSize) << std::dec << std::endl;      

    newProgram->loadIntoMemory(mem, programStartAddress, id);

    std::unordered_map<uint32_t, PageEntry> pageTable = mem.getPagingTable();
    for (const auto& [virtualAddr, page] : pageTable) 
    {
        if (virtualAddr >= programStartAddress && virtualAddr < (programStartAddress + alignedSize)) 
        {
            newProcess->workingSetPages.push_back(virtualAddr);
        }
    }
    
    scheduler.addProcess(newProcess);
    processList.push_back(newProcess);
}


void OperatingSystem::start() 
{
    std::cout << "Starting OS with Process and Program Management...\n";

    uint32_t usedMemory = currentMemAddress + sharedPages.size() * PAGE_SIZE;
    uint32_t totalMemory = mem.get_size();
    uint32_t remainingMemory = totalMemory - usedMemory;

    std::cout << "[MEMORY] Total memory: " << totalMemory << " bytes\n";
    std::cout << "[MEMORY] Used memory (programs + shared): " << usedMemory << " bytes\n";
    std::cout << "[MEMORY] Remaining memory: " << remainingMemory << " bytes\n";
        
    std::cout << "[MEMORY] Shared memory regions mapped at: \n";
    for (size_t i = 0; i < sharedPages.size(); ++i) 
    {
        std::cout << "  SharedPage " << i << ": 0x" << std::hex << sharedPages[i] << std::dec << "\n";
    }

    std::vector<PCB*> terminatedProcesses;

    while (!processList.empty()) 
    {
        std::cout << "\n[DEBUG] -------- Start of loop --------\n";
        std::cout << "[DEBUG] Process list size: " << processList.size() << std::endl;

        for (PCB* p : processList) 
        {
            if (p->state == "Sleeping") 
            {
                p->sleepCounter--;
                std::cout << "[SLEEPING] Process " << p->processId 
                  << " is sleeping... " 
                  << p->sleepCounter << " cycles remaining\n";
                if (p->sleepCounter <= 0) 
                {
                    p->state = "Ready";
                    scheduler.addProcess(p);
                    std::cout << "[WAKE UP] Process " << p->processId << " has finished sleeping and is now Ready\n";
                }
            }
        }

        PCB* next = scheduler.getNextProcess();
        if (!next) 
        {
            bool sleepingExists = false;
            for (PCB* p : processList) 
            {
                if (p->state == "Sleeping") 
                {
                    sleepingExists = true;
                    break;
                }
            }

            if (sleepingExists) 
            {
                std::cout << "[OS] No ready processes, but some are sleeping.\n";
                continue; 
            }
            std::cout << "[OS] No more processes to run. Halting...\n";
            break;
        }


        std::cout << "[DEBUG] Got next process: " << next->processId << std::endl;

        bool signFlag = cpu.getSignFlag();
        bool zeroFlag = cpu.getZeroFlag();
        next->restoreState(cpu.getRegisters(), signFlag, zeroFlag);
        cpu.setSignFlag(signFlag);
        cpu.setZeroFlag(zeroFlag);
        cpu.setRegister(12, next->processId);


        std::cout << "[DEBUG] CPU::run() start\n";
        std::cout << "[DEBUG] CPU IP before run(): " << std::hex << cpu.getInstructionPointer() << std::endl;

        cpu.run();
        
        if (cpu.isSleepRequested()) 
        {
            std::cout << "[DEBUG] CPU sleeping, exit run()\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->sleepCounter = cpu.getSleepDuration();
            std::cout << "[DEBUG] process is: " << next->state << std::endl;
            next->state = "Sleeping";
            std::cout << "[DEBUG] process is: " << next->state << std::endl;
            
            next->contextSwitches++; 
            cpu.clearSleepRequest();
            continue;
        }
        
        if (cpu.isTerminated()) 
        {
            next->state = "Terminated";
            std::cout << "[DEBUG] Process " << next->processId << " terminated via EXIT.\n";
            cpu.clearTerminatedFlag();

            processList.erase(std::remove(processList.begin(), processList.end(), next), processList.end());
            terminatedProcesses.push_back(next);
            
            next->contextSwitches++;
            continue;
        }

        signFlag = cpu.getSignFlag();
        zeroFlag = cpu.getZeroFlag();
        scheduler.switchProcess(cpu.getRegisters(), signFlag, zeroFlag);
        cpu.setSignFlag(signFlag);
        cpu.setZeroFlag(zeroFlag);

        std::cout << "[DEBUG] End of loop iteration.\n";
    }

    std::cout << "\n=== Process Statistics ===\n";

    for (PCB* process : processList) 
    {
        std::cout << "Process " << process->processId << " executed "
                << process->clockCycles << " cycles, "
                << process->contextSwitches << " context switches, "
                << "Final state: " << process->state << "\n";
        
        std::cout << "Pages used (virtual addresses):\n";
        for (size_t i = 0; i < process->workingSetPages.size(); ++i) 
        {
                std::cout << "  Page " << i << ": 0x" << std::hex << process->workingSetPages[i] << std::dec << "\n";
        }        
        
        std::cout << "Heap Allocations:\n";
        for (const auto& hp : process->heapAllocations) 
        {
            std::cout << "  Addr: 0x" << std::hex << hp.addr 
                    << ", Size: " << std::dec << hp.size 
                    << ", Used: " << (hp.used ? "Yes" : "No") << std::endl;
        }


        delete process;  
    }

    for (PCB* process : terminatedProcesses) 
    {
        std::cout << "Process " << process->processId << " executed "
                << process->clockCycles << " cycles, "
                << process->contextSwitches << " context switches, "
                << "Final state: " << process->state << "\n";
        
        std::cout << "Pages used (virtual addresses):\n";
        for (size_t i = 0; i < process->workingSetPages.size(); ++i) 
        {
            std::cout << "  Page " << i << ": 0x" << std::hex << process->workingSetPages[i] << std::dec << "\n";
        }  
        
        std::cout << "Heap Allocations:\n";
        for (const auto& hp : process->heapAllocations) 
        {
            std::cout << "  Addr: 0x" << std::hex << hp.addr 
                    << ", Size: " << std::dec << hp.size 
                    << ", Used: " << (hp.used ? "Yes" : "No") << std::endl;
        }
        delete process;  
    }
    mem.printPagingTable();
    mem.printMemoryMetrics();


    
}








    










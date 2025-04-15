#include "OperatingSystem.h"
#include <iostream>
#include <algorithm>

OperatingSystem::OperatingSystem(uint32_t memorySize, const std::string& programFile)
    : mem(memorySize), cpu(mem, scheduler), programFile(programFile) {
    currentMemAddress = 0;
        
    loadProcess(999, programFile, 4, 512, 1, 512); // load idle process with lowest priority
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

    uint32_t usedMemory = currentMemAddress;
    uint32_t totalMemory = mem.get_size();
    uint32_t remainingMemory = totalMemory - usedMemory;

    std::cout << "[MEMORY] Total memory: " << totalMemory << " bytes\n";
    std::cout << "[MEMORY] Used memory (programs + shared): " << usedMemory << " bytes\n";
    std::cout << "[MEMORY] Remaining memory: " << remainingMemory << " bytes\n";
        
    std::cout << "[MEMORY] Shared memory regions mapped at: \n";

    PCB* idleProcess = nullptr;
    for (PCB* p : processList) 
    {
        if (p->processId == 999) 
        {
            idleProcess = p;
            break;
        }
    }

    cpu.onCycleCallback = [&]() 
    {
        for (PCB* p : processList)
        {
            if (p->state == "Sleeping" && p->sleepCounter > 0)
            {
                p->sleepCounter--;
                std::cout << "[SLEEP CYCLE] Process " << p->processId 
                          << " now has " << p->sleepCounter << " cycles left\n";
                if (p->sleepCounter == 0)
                {
                    p->state = "Ready";
                    scheduler.addProcess(p);
                    std::cout << "[WAKE UP] Process " << p->processId << " is now Ready\n";
                }
            }
        }
    };

    while (!processList.empty()) 
    {
        if (processList.size() == 1 && processList[0]->processId == 999) 
        {
            std::cout << "[OS] Only idle process remains. Halting system...\n";
            break;
        }
        
        std::cout << "\n[DEBUG] -------- Start of loop --------\n";
        std::cout << "[DEBUG] Process list size: " << processList.size() << std::endl;


        PCB* next = scheduler.getNextProcess();

        if (!next || next->processId == 999) 
        {
            bool allBlocked = true;
            int minSleep = INT32_MAX;

            for (PCB* p : processList) 
            {
                if (p->state != "Sleeping" && p->state != "Waiting" &&
                    p->state != "Terminated" && p->processId != 999) 
                {
                    allBlocked = false;
                    break;
                }

                if (p->state == "Sleeping" && p->sleepCounter > 0 && p->processId != 999) 
                {
                    minSleep = std::min(minSleep, static_cast<int>(p->sleepCounter));
                }
            }

            std::cout << "[BLOCKED CHECK]: " << std::boolalpha << allBlocked << std::endl;

            if (allBlocked && idleProcess && idleProcess->state != "Terminated") 
            {
                next = idleProcess;
                if (minSleep != INT32_MAX) 
                {
                    next->settimeQuantum(minSleep);
                } 
                else 
                {
                    next->settimeQuantum(1);  // fallback
                }
                std::cout << "[IDLE] Running idle process for " << minSleep << " cycles...\n";
            } 
            else 
            {
                std::cout << "[OS] No more processes to run. Halting...\n";
                break;
            }
        }

        if (!next->isLoaded) 
        {
            next->program->loadIntoMemory(mem, next->registers[11], next->getPid());
            next->isLoaded = true;
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

        std::cout << "[DEBUGYUH] NUMBER OF CYCLES: " << next->clockCycles 
                  << " for process: " << next->processId << std::endl;
       
        if (next->processId == 999)
        {
            std::cout << "[IDLE CLEANUP] Deallocating memory used by idle process...\n";
            mem.deallocateProcessPages(next->processId);
            next->workingSetPages.clear();
        }          
        
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
            cpu.clearTerminatedFlag();

            std::cout << "\n[PROCESS TERMINATED] PID: " << next->processId << "\n";
            std::cout << "  → Clock Cycles: " << next->clockCycles << "\n";
            std::cout << "  → Context Switches: " << next->contextSwitches + 1 << "\n";  // +1 since it's ending
            int processPageFaults = 0;
            auto table = mem.getPagingTable();
            for (const auto& [virtAddr, entry] : table) {
                if (entry.pid == next->processId && entry.isValid)
                    processPageFaults++;
            }
            std::cout << "  → Page Faults: " << processPageFaults << "\n";

            std::cout << "  → Pages Used:\n";
            for (const auto& [virtAddr, entry] : table) {
                if (entry.pid == next->processId) {
                    std::cout << "    Virtual Page: 0x" << std::hex << virtAddr
                            << " | Physical Page: " << std::dec << entry.physicalPage
                            << " | IsValid: " << std::boolalpha << entry.isValid
                            << " | IsDirty: " << entry.isDirty
                            << " | LastUsed: " << entry.lastUsedTime << "\n";
                }
            }

            std::cout << "  → Heap Allocations:\n";
            for (const auto& hp : next->heapAllocations) {
                std::cout << "    Addr: 0x" << std::hex << hp.addr
                        << ", Size: " << std::dec << hp.size
                        << ", Used: " << (hp.used ? "Yes" : "No") << "\n";
            }

            mem.deallocateProcessPages(next->processId);
            next->workingSetPages.clear();

            processList.erase(std::remove(processList.begin(), processList.end(), next), processList.end());
            
            next->contextSwitches++;

            mem.debugPrintFreePages();

            continue;
        }

        if (cpu.isWaitingOnLock())
        {
            std::cout << "[DEBUG] CPU waiting on lock, exit run()\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Waiting";
            std::cout << "[DEBUG] Process " << next->processId << " is now Waiting for Lock\n";
            next->contextSwitches++; 
            cpu.clearWaitingOnLock();
            continue;
        }

        if (cpu.isExpired()) 
        {
            std::cout << "[DEBUG] Time quantum expired for Process " << next->processId << "\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Ready";
            scheduler.addProcess(next);  // Put back into ready queue
            next->contextSwitches++;
            cpu.clearExpiredFlag();
            continue;  // Go to next iteration of the loop
        }

        signFlag = cpu.getSignFlag();
        zeroFlag = cpu.getZeroFlag();
        scheduler.switchProcess(cpu.getRegisters(), signFlag, zeroFlag);
        cpu.setSignFlag(signFlag);
        cpu.setZeroFlag(zeroFlag);

        std::cout << "[DEBUG] End of loop iteration.\n";
    }

    mem.printPagingTable();
    mem.printMemoryMetrics();
   
}








    










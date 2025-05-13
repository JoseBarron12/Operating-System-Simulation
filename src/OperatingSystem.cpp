// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "OperatingSystem.h"
#include <iostream>
#include <algorithm>

/**
 * Constructs the Operating System instance, initializing memory, CPU,
 * and scheduler. Also loads the idle process from the specified program file.
 *
 * @param memorySize The total size of the simulated memory.
 * @param programFile The program file to use for the idle process.
 */
OperatingSystem::OperatingSystem(uint32_t memorySize, const std::string& programFile)
    : mem(memorySize), cpu(mem, scheduler), programFile(programFile) {
    currentMemAddress = 0;
        
    loadIdleProcess(programFile);
}

/**
 * Loads the special idle process into memory. This process is used when
 * no other runnable process exists. It is loaded into a fixed virtual address.
 *
 * @param file The path to the idle process binary file.
 */
void OperatingSystem::loadIdleProcess(const std::string& file)
{
    const uint32_t IDLE_VADDR = 0xF000;

    Program* idle = new Program(file);
    idle->loadProgram(file);

    // Create PCB for idle process
    PCB* idlePCB = new PCB(999, idle->getCodeSize(), 0, 0, 1, idle, 0);
    idlePCB->registers[11] = IDLE_VADDR;  // Set instruction pointer to fixed virtual address
    idlePCB->isLoaded = true;
    scheduler.addProcess(idlePCB);
    processList.push_back(idlePCB);


    // Load instructions into reserved memory using getaddress
    std::vector<uint32_t> code = idle->getInstructions();
    uint32_t addr = IDLE_VADDR;

    
    //std::cout << "[IDLE] Writing " << code.size() << " instruction values\n";
    //std::cout << "[IDLE WRITE] Writing to VA: 0x" << std::hex << addr << std::dec << "\n";

    for (size_t i = 0; i < code.size(); i += 3) 
    {
        uint32_t physical_addr = mem.getaddress(addr, 999); 
        
        //std::cout << "  → physical address: 0x" << std::hex << physical_addr << std::dec << "\n";

        mem.set8(physical_addr, code[i]);
        mem.set32(physical_addr + 1, code[i+1]); 
        mem.set32(physical_addr + 5, code[i+2]); 

        addr += 9; 
    }
    
}

/**
 * Loads a new user process into the system.
 *
 * @param id         Process ID.
 * @param programFile The path to the program's binary file.
 * @param stackSize  Size of the stack segment.
 * @param dataSize   Size of the data segment.
 * @param priority   Priority of the process.
 * @param heapSize   Size of the heap segment.
 */
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
    //std::cout << "[DEBUG] Set PCB IP to: " << std::hex << newProcess->registers[11] << std::endl;     

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

/**
 * Starts the operating system's scheduling loop. Handles dispatching,
 * CPU execution, state transitions, and memory management.
 */
void OperatingSystem::start() 
{
    std::cout << "Starting OS...\n";

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
                    
                    if (p->priority > p->processId) 
                    {
                        std::cout << "[PREEMPTION] Higher-priority process " << p->processId 
                                  << " is waking up. Forcing CPU to yield.\n";
                        cpu.triggerPreemption();
                    }
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
        
        //std::cout << "\n[DEBUG] -------- Start of loop --------\n";
        //std::cout << "[DEBUG] Process list size: " << processList.size() << std::endl;


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
            try 
            {
                next->program->loadIntoMemory(mem, next->registers[11], next->getPid());
                next->isLoaded = true;
            } 
            catch (const std::exception& e) // always means a blocked request
            {
                
                if (std::string(e.what()) == "BLOCKED_PAGE_FAULT")
                {
                    next->state = "Waiting";
                    next->isBlocked = true;
                    std::cout << "[MEMORY BLOCK] Process " << next->processId << " is now Waiting for Memory\n";
                    continue;
                }
                  
            }
             
        }


        //std::cout << "[DEBUG] Got next process: " << next->processId << std::endl;

        bool signFlag = cpu.getSignFlag();
        bool zeroFlag = cpu.getZeroFlag();
        next->restoreState(cpu.getRegisters(), signFlag, zeroFlag);
        cpu.setSignFlag(signFlag);
        cpu.setZeroFlag(zeroFlag);
        cpu.setRegister(12, next->processId);


        //std::cout << "[DEBUG] CPU::run() start\n";
        //std::cout << "[DEBUG] CPU IP before run(): " << std::hex << cpu.getInstructionPointer() << std::endl;

        cpu.run();

        //std::cout << "[DEBUGYUH] NUMBER OF CYCLES: " << next->clockCycles 
        //          << " for process: " << next->processId << std::endl;
       
        
        if (cpu.isPreempting())
        {
            //std::cout << "[DEBUG] CPU preempted. Re-scheduling immediately.\n";
            cpu.clearPreemption();

            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Ready";
            scheduler.addProcess(next);
            continue;  
        }
                  
        
        if (cpu.isSleepRequested()) 
        {
            //std::cout << "[DEBUG] CPU sleeping, exit run()\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->sleepCounter = cpu.getSleepDuration();
            //std::cout << "[DEBUG] process is: " << next->state << std::endl;
            next->state = "Sleeping";
            std::cout << "[OS] Process " << next->processId << " is: " << next->state << " for " << next->sleepCounter << " cycles"<< std::endl;
            
            next->contextSwitches++; 
            cpu.clearSleepRequest();
            
            
            continue;
        }
        
        if (cpu.isTerminated()) 
        {
            next->state = "Terminated";
            cpu.clearTerminatedFlag();
            next->totalClockCycles += next->clockCycles;

            std::cout << "\n[PROCESS TERMINATED] PID: " << std::dec << next->processId << " | Priority: " << std::dec << next->priority << "\n";
            std::cout << "  → Clock Cycles: " << std::dec << next->totalClockCycles << "\n";
            std::cout << "  → Context Switches: " << std::dec << next->contextSwitches + 1 << "\n";  // +1 since it's ending
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

            unblockProcesses();

            continue;
        }

        if (cpu.isWaitingOnLock())
        {
            //std::cout << "[DEBUG] CPU waiting on lock, exit run()\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Waiting";
            //std::cout << "[DEBUG] Process " << next->processId << " is now Waiting for Lock\n";
            next->contextSwitches++; 
            cpu.clearWaitingOnLock();
            
            continue;
        }

        if (cpu.isWaitingOnEvent()) 
        {
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Waiting";
            //std::cout << "[DEBUG] Process " << next->processId << " is now Waiting for Event\n";
            next->contextSwitches++; 
            cpu.clearWaitingOnEvent();
            
            continue;
        }
        
        if (cpu.isExpired()) 
        {
            std::cout << "[OS] Time quantum expired for Process " << next->processId << "\n";
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Ready";
            scheduler.addProcess(next);  // Put back into ready queue
            next->contextSwitches++;
            cpu.clearExpiredFlag();
            next->totalClockCycles += next->clockCycles;
            next->clockCycles = 0;
            continue;  // Go to next iteration of the loop
        }
        
        if(cpu.isBlocked())
        {
            next->saveState(cpu.getRegisters(), cpu.getSignFlag(), cpu.getZeroFlag());
            next->state = "Waiting";
            next->contextSwitches++; 
            cpu.clearBlocked();
            next->isBlocked = true;
            
            continue;
        }
        
        signFlag = cpu.getSignFlag();
        zeroFlag = cpu.getZeroFlag();
        scheduler.switchProcess(cpu.getRegisters(), signFlag, zeroFlag);
        cpu.setSignFlag(signFlag);
        cpu.setZeroFlag(zeroFlag);

        //std::cout << "[DEBUG] End of loop iteration.\n";

        
    }

    //mem.printPagingTable();
    //mem.printMemoryMetrics();
   
}

/**
 * Terminates a process by its PID and deallocates its resources.
 *
 * @param pid Process ID to terminate.
 * @return true if the process was found and terminated; false otherwise.
 */
bool OperatingSystem::terminateProcessByPid(uint32_t pid)
{
    auto it = std::find_if(processList.begin(), processList.end(),
        [pid](PCB* p) { return p->processId == pid; });

    if (it != processList.end()) 
    {
        PCB* next = *it;
        next->state = "Terminated";
        cpu.clearTerminatedFlag();
        next->totalClockCycles += next->clockCycles;
        std::cout << "[PROCESS KILLED] PID: " << std::dec << next->processId << " terminated by another process\n";
        std::cout << "\n[PROCESS TERMINATED] PID: " << std::dec << next->processId << "\n";
        std::cout << "  → Clock Cycles: " << std::dec << next->totalClockCycles << "\n";
        std::cout << "  → Context Switches: " << std::dec << next->contextSwitches + 1 << "\n";  // +1 since it's ending
        int processPageFaults = 0;
        auto table = mem.getPagingTable();
        for (const auto& [virtAddr, entry] : table) 
        {
            if (entry.pid == next->processId && entry.isValid)
                    processPageFaults++;
        }
        
        std::cout << "  → Page Faults: " << processPageFaults << "\n";

        std::cout << "  → Pages Used:\n";
        for (const auto& [virtAddr, entry] : table) 
        {
            if (entry.pid == next->processId)
            {
                std::cout << "    Virtual Page: 0x" << std::hex << virtAddr
                          << " | Physical Page: " << std::dec << entry.physicalPage
                          << " | IsValid: " << std::boolalpha << entry.isValid
                          << " | IsDirty: " << entry.isDirty
                          << " | LastUsed: " << entry.lastUsedTime << "\n";
                
            }
        }

        std::cout << "  → Heap Allocations:\n";
        for (const auto& hp : next->heapAllocations) 
        {
            std::cout << "    Addr: 0x" << std::hex << hp.addr
                      << ", Size: " << std::dec << hp.size
                      << ", Used: " << (hp.used ? "Yes" : "No") << "\n";
        }

        mem.deallocateProcessPages(next->processId);
        next->workingSetPages.clear();
        processList.erase(it);

        unblockProcesses();
                    

        return true;
    }
    return false;
}

/**
 * Checks if there are any processes that are sleeping or waiting.
 *
 * @return true if such processes exist (excluding the idle process), false otherwise.
 */
bool OperatingSystem::hasSleepingOrWaitingProcesses() 
{
    for (auto* p : processList) 
    {
        if ((p->state == "Sleeping" || p->state == "Waiting") && p->processId != 999)
            return true;
    }
    return false;
}

/**
 * Checks if all non-idle processes are either sleeping or waiting.
 *
 * @return true if all non-idle processes are in a non-runnable state, false otherwise.
 */
bool OperatingSystem::hasAllSleepingOrWaitingProcesses()
{
    for (auto* p : processList) 
    {
        if (p->processId != 999 && p->state != "Sleeping" && p->state != "Waiting")
        {
            return false;
        }
    }
    return true;
}

/**
 * Unblocks the highest-priority process that is waiting on memory.
 * Loads the process back into memory and moves it to the ready state.
 */
void OperatingSystem::unblockProcesses()
{
    PCB* highestPriorityWaiting = nullptr;

    for (PCB* p : processList)
    {
        if (p->state == "Waiting" && p->isBlocked)
        {
            if (!highestPriorityWaiting || p->priority > highestPriorityWaiting->priority)
            {
                highestPriorityWaiting = p;
            }
        }
    }

    if (highestPriorityWaiting && !highestPriorityWaiting->isLoaded)
    {
        
        highestPriorityWaiting->program->loadIntoMemory(mem, highestPriorityWaiting->registers[11], highestPriorityWaiting->getPid());
        highestPriorityWaiting->isLoaded = true;
        highestPriorityWaiting->isBlocked = false;
        highestPriorityWaiting->state = "Ready";
        scheduler.addProcess(highestPriorityWaiting);
        std::cout << "[MEMORY UNBLOCK] Process " << highestPriorityWaiting->processId << " loaded and moved to Ready\n";
    }
    else if (highestPriorityWaiting)
    {
        highestPriorityWaiting->state = "Ready";
        highestPriorityWaiting->isBlocked = false;
        scheduler.addProcess(highestPriorityWaiting);
        std::cout << "[MEMORY UNBLOCK] Process " << highestPriorityWaiting->processId << " loaded and moved to Ready\n";
    }

}





    










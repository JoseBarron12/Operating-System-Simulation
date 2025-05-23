// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "cpu.h"
#include <unordered_map>
#include <algorithm>
#include "OperatingSystem.h" 

/** 
 * Constructs CPU, and initializes registers, locks, events, system clock, and flags.
 *
 * @param memRef   Reference to memory.
 * @param sched    Reference to the process scheduler.
 */
CPU::CPU(memory& memRef, ProcessScheduler& sched) : mem(memRef), scheduler(sched), systemClock(0), signFlag(false), zeroFlag(false)
{
    registers[11] = 0;  // Instruction Pointer
    registers[12] = 0;  // Current Process ID
    registers[13] = mem.get_size() - 1;  // Stack Pointer
    registers[14] = 0;  // Global Memory Start
    locks.resize(16, -1); // Locks
    eventStates.fill(false); // Events
}

/** 
 * Loads a program into memory starting at address 0.
 * 
 * @param program  Program to be loaded.
 */
void CPU::loadProgram(std::vector<uint8_t>& program)
{
    for (int i = 0; i < program.size(); i++)
    {
        mem.set8(i,program[i]);
    }
}

/**
 * Releases all locks currently held by the specified process ID and reassigns
 * them to the highest priority process waiting.
 *
 * @param pid  Process ID of the process releasing the locks.
 */
void CPU::releaseAllHeldLocks(uint32_t pid) 
{
    for (int i = 0; i < locks.size(); ++i) 
    {
        if (locks[i] == static_cast<int32_t>(pid)) 
        {
            locks[i] = -1;

            auto& queue = lockWaitQueues[i];
            if (!queue.empty()) 
            {
                auto it = std::max_element(queue.begin(), queue.end(), [](PCB* a, PCB* b)
                {
                    return a->priority < b->priority;
                });

                PCB* next = *it;
                queue.erase(it);
                locks[i] = next->getPid();
                next->state = "Ready";
                scheduler.addProcess(next);

                std::cout << "[LOCK] Lock " << i << " reassigned to process " << next->getPid() << " after " << pid << " exited\n";
            }
        }
    }
}

/**
 * Executes the next instruction of the currently running process, handling opcode
 * decoding, memory accesses, register operations, system calls, and control flow.
 * Includes error handling for invalid instructions and page faults.
 */

void CPU::executeNextInstruction()
{
    try 
    {


        uint32_t ip = registers[11]; // Instruction pointer
        /*
        if (ip >= mem.get_size()) 
        {
            std::cerr << "ERROR: Instruction Pointer (IP) out of range! IP: " 
                    << std::hex << ip << std::endl;
            exit(1);
        }*/
        
        // Process system and process clock cycles 
        systemClock++; 
        PCB* current = scheduler.getRunningProcess();
        if (current) 
        {
            current->incrementClockCycle();  
        }
        else 
        {
            std::cerr << "[CPU ERROR] No running process found! Cannot increment clock cycle.\n";
            terminated = true;
            return;
        }
        
        // Read instruction from memory
        uint8_t opcodeByte = mem.get8(ip, current->getPid());

        // Validate instruction byte
        if (opcodeByte == 0 || opcodeByte >= 0x34) 
        {
            std::cerr << "ERROR: Unknown opcode encountered: " 
                    << std::hex << static_cast<int>(opcodeByte) 
                    << " at " << ip << std::endl;
            terminated = true;
            return;
        }

        // Read arguments from memory
        uint32_t arg1 = mem.get32(ip + 1, current->getPid());
        uint32_t arg2 = mem.get32(ip + 5, current->getPid());
        
        bool shouldIncrementIP = true;

        // Process each opcode, and perform instruction
        switch (static_cast<Opcode>(opcodeByte))
        {
            case Opcode::Incr:
                if (arg1 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Incr: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                registers[arg1]++;
                break;

            case Opcode::Addi:
                if (arg1 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Addi: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                registers[arg1] += arg2;
                break;

            case Opcode::Addr:
                if (arg1 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Addr: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                registers[arg1] += registers[arg2];
                break;

            case Opcode::Pushr:
            {
                registers[13] -= 4;  // Decrement SP
                // Stack overflow check
                if (registers[13] < current->heapEnd) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                                  << " SP=0x" << std::hex << registers[13]
                                  << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                mem.set32(registers[13], registers[arg1]);
                break;
            }
                
            case Opcode::Pushi: 
            {
                registers[13] -= 4;
                // Stack overflow check
                if (registers[13] < current->heapEnd ) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                                << " SP=0x" << std::hex << registers[13]
                                << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true;  
                    return;
                }
                mem.set32(registers[13], arg1);
                break;
            }
                
            case Opcode::Movi:
                if (arg1 >= 15) 
                {
                    std::cerr << "[ERROR] Invalid register index in Movi: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                registers[arg1] = arg2;
                break;

            case Opcode::Movr:
                if (arg1 >= 15 || arg2 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Movr: " << arg1 << " or " << arg2 << "\n";
                    terminated = true;
                    return;
                }
                registers[arg1] = registers[arg2];
                break;

            case Opcode::Movmr:
            {
                if (arg1 >= 15 || arg2 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Movmr: " << arg1 << " or " << arg2 << "\n";
                    terminated = true;
                    return;
                }
                
                uint32_t vaddr = registers[arg2];
                uint32_t value = mem.get32(vaddr, current->getPid());
                registers[arg1] = value;
                break;
            }
                
            case Opcode::Movrm:
            {
                if (arg1 >= 15 || arg2 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Movrm: " << arg1 << " or " << arg2 << "\n";
                    terminated = true;
                    return;
                }
                
                uint32_t vaddr = registers[arg1];
                uint32_t value = registers[arg2];
                uint32_t physical_addr = mem.getaddress(registers[arg1], current->getPid());

                //std::cout << "[DEBUG MOVRM] Storing value " << value 
                //          << " from r" << arg2 << " into VA 0x" << std::hex << vaddr 
                //          << " (PA 0x" << physical_addr << ")" << std::dec << "\n";

                mem.set32(physical_addr, registers[arg2]);
                break;
            }
                
            case Opcode::Movmm:
                if (!mem.check_illegal(arg1) && !mem.check_illegal(arg2)) 
                {
                    mem.set32(mem.get32(arg1, current->getPid()), mem.get32(arg2, current->getPid())); 
                    
                }
                else
                {
                    std::cerr << "WARNING: Attempted MOVMM with invalid addresses: " 
                            << std::hex << arg1 << " or " << arg2 << std::endl;
                    terminated = true;
                    return;

                }
                
                break;

            case Opcode::Printr:
                if (arg1 < 16) 
                {  
                    std::cout << "Register " << arg1 << ": " << std::dec << registers[arg1] << std::endl;
                } 
                else 
                {
                    std::cerr << "WARNING: Invalid register accessed: " << arg1 << std::endl;
                    terminated = true;
                    return;
                }
                break;
            
            case Opcode::Printm:
                if (!mem.check_illegal(registers[arg1])) 
                {
                    std::cout << "Memory[" << registers[arg1] << "]: " 
                            << mem.get32(registers[arg1], current->getPid()) << std::endl;
                } 
                else 
                {
                    std::cerr << "WARNING: Attempted to print invalid memory at: " << registers[arg1] << std::endl;
                    terminated = true;
                    return;
                }
                break;
            
            case Opcode::Printcr:
                if (arg1 >= 15) 
                {
                    std::cerr << "[ERROR] Invalid register index in Printcr: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                std::cout << static_cast<char>(registers[arg1]) << std::endl;
                break;

            case Opcode::Printcm:
                if (arg1 >= 15) 
                {
                    std::cerr << "[ERROR] Invalid register index in Printcm: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                std::cout << static_cast<char>(mem.get8(registers[arg1], current->getPid())) << std::endl;
                break;

            case Opcode::Jmp:
                registers[11] += registers[arg1];
                shouldIncrementIP = false;
                break;

            case Opcode::Jmpi:
                registers[11] += arg1;
                shouldIncrementIP = false;
                break;

            case Opcode::Jmpa:
                registers[11] = arg1;
                shouldIncrementIP = false;
                break;

            case Opcode::Cmpi:
                if (registers[arg1] < arg2) signFlag = true;
                else if (registers[arg1] == arg2) zeroFlag = true;
                else signFlag = false, zeroFlag = false;
                break;

            case Opcode::Cmpr:
                if (registers[arg1] < registers[arg2]) signFlag = true;
                else if (registers[arg1] == registers[arg2]) zeroFlag = true;
                else signFlag = false, zeroFlag = false;
                break;

            case Opcode::Jlt:
                if (signFlag) {
                    registers[11] += registers[arg1];
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jlti:
                if (signFlag) {
                    registers[11] += arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jlta:
                if (signFlag) {
                    registers[11] = arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jgt:
                if (!signFlag) {
                    registers[11] += registers[arg1];
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jgti:
                if (!signFlag) {
                    registers[11] += arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jgta:
                if (!signFlag) {
                    registers[11] = arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Je:
                if (zeroFlag) {
                    registers[11] += registers[arg1];
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jei:
                if (zeroFlag) {
                    registers[11] += arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Jea:
                if (zeroFlag) {
                    registers[11] = arg1;
                    shouldIncrementIP = false;
                }
                break;

            case Opcode::Call:
            {
                registers[13] -= 4;
                // Stack overflow check
                if (registers[13] < current->heapEnd) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                            << " SP=0x" << std::hex << registers[13]
                            << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                uint32_t physical_addr = mem.getaddress(registers[13], current->getPid());
                mem.set32(physical_addr, registers[11] + 9);
                registers[11] += arg1; 
                shouldIncrementIP = false;
                break;
            }
            
            case Opcode::Callm:
                registers[13] -= 4;
                // Stack overflow check
                if (registers[13] < current->heapEnd) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                            << " SP=0x" << std::hex << registers[13]
                            << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                mem.set32(registers[13], registers[11]);
                registers[11] = registers[arg1];  
                shouldIncrementIP = false;
                break;

            case Opcode::Ret:
                
                // Stack overflow check
                if (registers[13] < current->heapEnd) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                            << " SP=0x" << std::hex << registers[13]
                            << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                registers[11] = mem.get32(registers[13], current->getPid());
                registers[13] += 4; 
                shouldIncrementIP = false;
                break;

            case Opcode::Exit:
                std::cout << "Program terminated." << std::endl;
                releaseAllHeldLocks(registers[12]);
                terminated = true;  // Add a flag in CPU
                return;
            
            case Opcode::Popr:   
                registers[13] += 4;
                // Stack overflow check
                if (registers[13] < current->heapEnd) 
                {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                            << " SP=0x" << std::hex << registers[13]
                            << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                registers[arg1] = mem.get32(registers[13], current->getPid());
                break;

            case Opcode::Popm:
                registers[13] += 4;
                // Stack overflow check
                if (registers[13] < current->heapEnd) {
                    std::cerr << "[STACK OVERFLOW] Process " << current->processId
                            << " SP=0x" << std::hex << registers[13]
                            << " went below heapEnd=0x" << current->heapEnd << std::dec << "\n";
                    terminated = true; 
                    return;
                }
                mem.set32(registers[arg1], mem.get32(registers[13], current->getPid()));
                break;

            case Opcode::Sleep:
                
                //std::cout << " [OS] CPU requested sleep for " << registers[arg1] << " cycles.\n";
                sleepRequested = true;
                sleepDuration = registers[arg1];

                registers[11] = ip + 9;
                return;

            case Opcode::Input:
                if (arg1 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Input: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                std::cin >> registers[arg1];
                break;

            case Opcode::Inputc:
                if (arg1 >= 15) {
                    std::cerr << "[ERROR] Invalid register index in Inputc: " << arg1 << "\n";
                    terminated = true;
                    return;
                }
                char c;
                std::cin >> c;
                registers[arg1] = static_cast<uint32_t>(c);
                break;

            case Opcode::SetPriority:
                if (registers[arg1] < 2 || registers[arg1] > 32) break;
                current->priority = registers[arg1];
                break;
            case Opcode::SetPriorityI:
                if (arg1 < 2 || arg1 > 32) break;
                current->priority = arg1;
                break;
            
            case Opcode::MapSharedMem:
                {
                    if (!current) break;
                
                    uint32_t sharedRegion = arg1;
                
                    if (sharedRegion < 0 || sharedRegion > 9)  // 0 to 9 for 10 regions
                    {
                        std::cerr << "[SHARED] Invalid shared memory region number: " << sharedRegion << "\n";
                        break;
                    }
                
                    if (mem.sharedMemoryTable.find(sharedRegion) == mem.sharedMemoryTable.end())
                    {
                        std::cerr << "[SHARED] Shared memory region " << sharedRegion << " does not exist!\n";
                        break;
                    }
                
                    uint32_t virtualSharedAddr = 0xFF00 + sharedRegion * PAGE_SIZE;
                    uint32_t physAddr = mem.sharedMemoryTable[sharedRegion].physicalPage;
                
                    mem.mapPage(virtualSharedAddr, physAddr / PAGE_SIZE, current->getPid());
                
                    registers[arg2] = virtualSharedAddr;
                    current->workingSetPages.push_back(virtualSharedAddr);
                
                    std::cout << "[SHARED] Process " << current->processId
                            << " mapped shared region " << sharedRegion
                            << " to virtual address 0x" << std::hex << virtualSharedAddr << std::dec << "\n";
                    break;
                }
                
            case Opcode::AcquireLock:
                {
                    uint32_t lockNum = registers[arg1];
                    if (lockNum < 1 || lockNum > 10) break;
                
                    if (locks[lockNum] == static_cast<int32_t>(current->getPid())) break;
                
                    if (locks[lockNum] == -1) 
                    {

                        locks[lockNum] = current->getPid();
                        std::cout << "[LOCK] Process " << current->getPid() << " acquired lock " << lockNum << "\n";
                    } 
                    else 
                    {
                        PCB* owner = scheduler.getProcessByPid(locks[lockNum]);
                        if (owner && owner->priority < current->priority) 
                        {
                            std::cout << "[PRIORITY INVERSION] Boosting priority of process " 
                                    << owner->processId << " from " << owner->priority 
                                    << " to " << current->priority << "\n";
                            owner->priority = current->priority;
                        }
                
                        current->state = "Waiting";
                        current->waitingForLock = true;
                        lockWaitQueues[lockNum].push_back(current);
                        std::cout << "[LOCK] Process " << current->getPid() << " waiting on lock " << lockNum << "\n";
                        waitingOnLock = true;
                        return; 
                    }
                    break;
                }
                
            case Opcode::AcquireLockI:
                {
                    if (arg1 < 1 || arg1 > 10) break;
                
                    if (locks[arg1] == static_cast<int32_t>(current->getPid())) break;
                
                    if (locks[arg1] == -1) 
                    {
                        locks[arg1] = current->getPid();
                        std::cout << "[LOCK] Process " << current->getPid() << " acquired lock " << arg1 << "\n";
                    } 
                    else 
                    {
                        PCB* owner = scheduler.getProcessByPid(locks[arg1]);
                        if (owner && owner->priority < current->priority) 
                        {
                            std::cout << "[PRIORITY INVERSION] Boosting priority of process " 
                                    << owner->processId << " from " << owner->priority 
                                    << " to " << current->priority << "\n";
                            owner->priority = current->priority;
                        }
                
                        current->state = "Waiting";
                        current->waitingForLock = true;
                        
                        lockWaitQueues[arg1].push_back(current);
                        std::cout << "[LOCK] Process " << current->getPid() << " waiting on lock " << arg1 << "\n";
                        waitingOnLock = true;
                        return; 
                    }
                    break;
                }
                
            case Opcode::ReleaseLock:
                {
                    uint32_t lockNum = registers[arg1];
                    if (lockNum < 1 || lockNum > 10) break;

                    if (locks[lockNum] != static_cast<int32_t>(current->getPid())) break;

                    locks[lockNum] = -1;

                    std::cout << "[LOCK] Process " << current->getPid() << " released lock " << lockNum << "\n";

                    auto& queue = lockWaitQueues[lockNum];
                    if (!queue.empty()) 
                    {
                        auto it = std::max_element(queue.begin(), queue.end(), [](PCB* a, PCB* b) {
                            return a->priority < b->priority;
                        });

                        PCB* next = *it;
                        queue.erase(it);
                        locks[lockNum] = next->getPid();
                        next->state = "Ready";
                        next->waitingForLock = false;
                        scheduler.addProcess(next);
                        std::cout << "[LOCK] Process " << next->getPid() << " granted lock " << lockNum << "\n";
                    }
                    break;
                }

            case Opcode::ReleaseLockI:
                {
                    if (arg1 < 1 || arg1 > 10) break;

                    if (locks[arg1] != static_cast<int32_t>(current->getPid())) break;

                    locks[arg1] = -1;

                    std::cout << "[LOCK] Process " << current->getPid() << " released lock " << arg1 << "\n";

                    auto& queue = lockWaitQueues[arg1];
                    if (!queue.empty()) 
                    {
                        auto it = std::max_element(queue.begin(), queue.end(), [](PCB* a, PCB* b) {
                            return a->priority < b->priority;
                        });

                        PCB* next = *it;
                        queue.erase(it);
                        locks[arg1] = next->getPid();
                        next->state = "Ready";
                        next->waitingForLock = false;
                        scheduler.addProcess(next);
                        std::cout << "[LOCK] Process " << next->getPid() << " granted lock " << arg1 << "\n";
                    }
                    break;
                }

            case Opcode::SignalEvent:
                {
                    uint32_t eventIndex = registers[arg1];
                    if (eventIndex < 10) 
                    {
                        eventStates[eventIndex] = true;
                        auto& waitQueue = eventWaitQueues[eventIndex];
                        for (PCB* p : waitQueue) 
                        {
                            p->state = "Ready";
                            scheduler.addProcess(p);
                            std::cout << "[EVENT] Waking process " << p->processId << " waiting on event " << eventIndex << "\n";
                        }
                        waitQueue.clear();
                        std::cout << "[EVENT] Signaled event " << eventIndex << "\n";
                    }
                    break;
                }  
            case Opcode::SignalEventI:
                {
                    if (arg1 < 10) 
                    {
                        eventStates[arg1] = true;
                        auto& waitQueue = eventWaitQueues[arg1];
                        for (PCB* p : waitQueue) 
                        {
                            p->state = "Ready";
                            scheduler.addProcess(p);
                            std::cout << "[EVENT] Waking process " << p->processId << " waiting on event " << arg1 << "\n";
                        }
                        waitQueue.clear();
                        std::cout << "[EVENT] Signaled event " << arg1 << "\n";
                    }
                    break;
                }  
            case Opcode::WaitEvent:
                {
                    uint32_t eventIndex = registers[arg1];
                    if (eventIndex < 10) 
                    {
                        if (!eventStates[eventIndex])
                        {
                            PCB* current = scheduler.getRunningProcess();
                            current->state = "Waiting";
                            eventWaitQueues[eventIndex].push_back(current);
                            std::cout << "[EVENT] Process " << current->processId << " waiting on event " << eventIndex << "\n";
                            requestEventWait();
                            return; // yield for context switch
                        }
                    }
                    break;
                }
                
            case Opcode::WaitEventI:
                {
                    if (arg1 < 10) 
                    {
                        if (!eventStates[arg1]) 
                        {
                            PCB* current = scheduler.getRunningProcess();
                            current->state = "Waiting";
                            eventWaitQueues[arg1].push_back(current);
                            std::cout << "[EVENT] Process " << current->processId << " waiting on event " << arg1 << "\n";
                            requestEventWait();
                            return; // yield for context switch
                        }
                    }
                    break;
                }

                case Opcode::Alloc:
                {
                    if (!current) break;
                
                    uint32_t requestedSize = registers[arg1];
                    std::cout << "REQUESTED SIZE: " << requestedSize << std::endl;
                    std::cout << "HEAPSIZE: " << current->heapSize << std::endl;
                
                    if (requestedSize <= current->heapSize) 
                    {
                        uint32_t virtualAddr = current->heapEnd;
                
                        registers[arg2] = virtualAddr;
                
                        current->heapAllocations.push_back({virtualAddr, requestedSize, true});
                
                        current->heapEnd = virtualAddr + requestedSize;
                
                        std::cout << "[HEAP] Process " << current->processId 
                                << " allocated " << requestedSize << " bytes at 0x" 
                                << std::hex << virtualAddr << std::dec << "\n";
                    }
                    else 
                    {
                        registers[arg2] = 0;
                        std::cerr << "[HEAP] Allocation failed for Process " << current->processId 
                                << ": not enough space.\n";
                    }
                    break;
                }            
            case Opcode::FreeMemory:
                {
                    if (!current) break;
                    uint32_t addr = registers[arg1];
                    uint32_t heapStart = current->heapStart;
                    uint32_t heapEnd = current->heapEnd;
                
                    if (addr >= heapStart && addr < heapEnd) 
                    {
                        bool found = false;
                        for (auto& page : current->heapAllocations) 
                        {
                            if (page.addr == addr && page.used) 
                            {
                                page.used = false;
                                std::cout << "[HEAP] Process " << current->processId 
                                        << " freed memory starting at address 0x" << std::hex << addr << std::dec << "\n";
                                found = true;
                                break;
                            }
                        }
                
                        if (!found) 
                        {
                            std::cerr << "[HEAP] Address 0x" << std::hex << addr << std::dec 
                                    << " is not currently allocated (Process " << current->processId << ")\n";
                        }
                    } 
                    else 
                    {
                        std::cerr << "[HEAP] Invalid free address 0x" << std::hex << addr << std::dec 
                                << " by process " << current->processId << "\n";
                    }
                    break;
                }
                case Opcode::TerminateProcess:
                {
                    uint32_t targetPid = registers[arg1];
                    if (os->terminateProcessByPid(targetPid)) 
                    {
                        std::cout << "[TERMINATE OPCODE] Process " << current->getPid()
                                << " terminated Process " << targetPid << "\n";
                    } else 
                    {
                        std::cout << "[TERMINATE OPCODE] Failed to terminate Process " << targetPid << "\n";
                    }
                    break;
                }

            
            default:
                std::cerr << "Unknown opcode: " << static_cast<int>(opcodeByte) << std::endl;
                break;
        }

        // Move to next instruction if allowed
        if (shouldIncrementIP) 
        {
            registers[11] = ip + 9;
        }

        // Check for quantum cycle expiration
        if ( !terminated && current->clockCycles >= current->timeQuantum)
        {
            systemClock = 0;
            quantumExpired = true;
            

        }
    }
    catch (const std::exception& e) // Handle errors during cpu execution
    {
        if (std::string(e.what()) == "BLOCKED_PAGE_FAULT") // Handle blocked memory
        {
            PCB* current = scheduler.getRunningProcess();
            if (current)
            {
                std::cout << "[MEMORY BLOCK] Process " << current->processId << " is now Waiting for Memory\n";
                blockedOnMemory = true;
            }
            
        }
        else // Handle out of memory error
        {
            PCB* current = scheduler.getRunningProcess();
            if (current)
            {
                std::cerr << "[CPU ERROR] " << e.what() << "\n";
                std::cerr << "[CPU STATE] Terminating process " << current->getPid() << "\n";
                for (int i = 0; i < 15; ++i)
                {
                    std::cerr << "  Register[" << i << "] = 0x" << std::hex << registers[i] << std::dec << "\n";
                }

                terminated = true;
            }
        }
    }
}

/**
 * Main execution loop of the CPU. Repeatedly calls executeNextInstruction until
 * it is terminated or yields control through preemption, sleep, or wait
 */
void CPU::run() 
{
    //std::cout << "[DEBUG] IP: " << std::hex << registers[11] << std::endl;
    
    quantumExpired = false;

    while (true) 
    {
        executeNextInstruction();

        if (onCycleCallback) onCycleCallback(); 

        if (preemptNow) 
        {
            //std::cout << "[DEBUG] CPU preempted by higher-priority process, exit run()\n";
            return;
        }

        if (terminated) 
        {
            //std::cout << "[DEBUG] CPU termination flag set, exit run()\n";
            return;
        }
        
        if (sleepRequested)
        {
            return;
        }
        
        if (quantumExpired)
        {
            //std::cout << "[DEBUG] CPU expiration flag set, exit run()\n";
            return;
        }
        if (waitingOnLock) 
        {
            //std::cout << "[DEBUG] CPU waiting on lock, exit run()\n";
            return;
        }
        if (waitingOnEvent) 
        {
            //std::cout << "[DEBUG] CPU waiting on event, exit run()\n";
            return;
        }
        if (blockedOnMemory)
        {
            return;
        }
    }
}





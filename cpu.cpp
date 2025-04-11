#include "cpu.h"

CPU::CPU(memory& memRef, ProcessScheduler& sched) : mem(memRef), scheduler(sched), systemClock(0), signFlag(false), zeroFlag(false)
{
    registers[11] = 0;  // Instruction Pointer
    registers[12] = 0;  // Current Process ID
    registers[13] = mem.get_size() - 1;  // Stack Pointer
    registers[14] = 0;  // Global Memory Start
    locks.resize(16, -1); 
    eventStates.fill(false);
}

void CPU::loadProgram(std::vector<uint8_t>& program)
{
    for (int i = 0; i < program.size(); i++)
    {
        mem.set8(i,program[i]);
    }
}

void CPU::releaseAllHeldLocks(uint32_t pid)
{
    for (int i = 0; i < locks.size(); i++)
    {
        if (locks[i] == static_cast<int32_t>(pid))
        {
            locks[i] = -1;
        }
    }
}


void CPU::executeNextInstruction()
{
    uint32_t ip = registers[11];

    /*
    if (ip >= mem.get_size()) 
    {
        std::cerr << "ERROR: Instruction Pointer (IP) out of range! IP: " 
                  << std::hex << ip << std::endl;
        exit(1);
    }*/
    
    systemClock++; 
    PCB* current = scheduler.getRunningProcess();
    if (current) 
    {
        current->incrementClockCycle();  
    }

    uint8_t opcodeByte = mem.get8(ip, current->getPid());


    if (opcodeByte == 0 || opcodeByte >= 0x33) 
    {
        std::cerr << "ERROR: Unknown opcode encountered: " 
                  << std::hex << static_cast<int>(opcodeByte) 
                  << " at " << ip << std::endl;
        exit(1);
    }

    uint32_t arg1 = mem.get32(ip + 1, current->getPid());
    uint32_t arg2 = mem.get32(ip + 5, current->getPid());
    
    bool shouldIncrementIP = true;

    
  
    switch (static_cast<Opcode>(opcodeByte))
    {
        case Opcode::Incr:
            registers[arg1]++;
            break;

        case Opcode::Addi:
            registers[arg1] += arg2;
            break;

        case Opcode::Addr:
            registers[arg1] += registers[arg2];
            break;

        case Opcode::Pushr:
            registers[13] -= 4;  // Decrement stack pointer first
            mem.set32(registers[13], registers[arg1]);  
            break;

        case Opcode::Pushi:
            registers[13] -= 4;  
            mem.set32(registers[13], arg1);
            break;

        case Opcode::Movi:
            registers[arg1] = arg2;
            break;

        case Opcode::Movr:
            registers[arg1] = registers[arg2];
            break;

        case Opcode::Movmr:
            registers[arg1] = mem.get32(registers[arg2], current->getPid());
            break;

        case Opcode::Movrm:
            mem.set32(registers[arg1], registers[arg2]);
            break;

        case Opcode::Movmm:
            if (!mem.check_illegal(arg1) && !mem.check_illegal(arg2)) 
            {
                mem.set32(mem.get32(arg1, current->getPid()), mem.get32(arg2, current->getPid())); 
                std::cerr << "WARNING: Attempted MOVMM with invalid addresses: " 
                          << std::hex << arg1 << " or " << arg2 << std::endl;
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
            }
            break;
        
        case Opcode::Printcr:
            std::cout << static_cast<char>(registers[arg1]) << std::flush;  
            break;

        case Opcode::Printcm:
            std::cout << static_cast<char>(mem.get8(registers[arg1], current->getPid())) << std::flush;  
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
            registers[13] -= 4;
            mem.set32(registers[13], registers[11]);
            registers[11] += registers[arg1]; 
            shouldIncrementIP = false;
            break;

        case Opcode::Callm:
            registers[13] -= 4;
            mem.set32(registers[13], registers[11]);
            registers[11] = registers[arg1];  
            shouldIncrementIP = false;
            break;

        case Opcode::Ret:
            registers[13] += 4;  
            registers[11] = mem.get32(registers[13], current->getPid());
            shouldIncrementIP = false;
            break;

        case Opcode::Exit:
            std::cout << "Program terminated." << std::endl;
            releaseAllHeldLocks(registers[12]);
            terminated = true;  // Add a flag in CPU
            return;
          
        case Opcode::Popr:
            registers[13] += 4;
            registers[arg1] = mem.get32(registers[13], current->getPid());
            break;

        case Opcode::Popm:
            registers[13] += 4;
            mem.set32(registers[arg1], mem.get32(registers[13], current->getPid()));
            break;

        case Opcode::Sleep:
            
            std::cout << "[DEBUG] CPU requested sleep for " << registers[arg1] << " cycles.\n";
            sleepRequested = true;
            sleepDuration = registers[arg1];

            registers[11] = ip + 9;
            return;

        case Opcode::Input:
            std::cin >> registers[arg1];
            break;

        case Opcode::Inputc:
            char c;
            std::cin >> c;
            registers[arg1] = static_cast<uint32_t>(c);
            break;

        case Opcode::SetPriority:
        case Opcode::SetPriorityI:
            std::cerr << "Warning: SetPriority not implemented in CPU." << std::endl;
            break;
        case Opcode::MapSharedMem:
            {
                if (!current) break;

                uint32_t virtualSharedAddr = 0xF000 + arg1 * PAGE_SIZE;

                if (!mem.pageExists(virtualSharedAddr)) 
                {
                    std::cerr << "ERROR: Shared memory page not mapped globally at 0x" 
                            << std::hex << virtualSharedAddr << std::dec << std::endl;
                    break;
                }

                uint32_t physPage = mem.getPagingTable().at(virtualSharedAddr).physicalPage;
                mem.mapPage(virtualSharedAddr, physPage, current->getPid());
                registers[arg2] = virtualSharedAddr;
                current->workingSetPages.push_back(virtualSharedAddr);

                std::cout << "[SHARED] Process " << current->processId
                        << " mapped shared page " << arg1
                        << " at virtual address 0x" << std::hex << virtualSharedAddr << std::dec << std::endl;
                break;
            }
            
        case Opcode::AcquireLock:
            {
                uint32_t lockNum = registers[arg1];
                if (lockNum < locks.size() && locks[lockNum] == -1)
                {
                    locks[lockNum] = registers[12]; 
                    std::cout << "[LOCK] Process " << registers[12] << " acquired lock " << lockNum << std::endl;
                }
                break;
            }
        case Opcode::AcquireLockI:
            {
                if (arg1 < locks.size() && locks[arg1] == -1)
                {
                    locks[arg1] = registers[12];
                    std::cout << "[LOCK] Process " << registers[12] << " acquired lock " << arg1 << std::endl;
                }
                break;
            }
        case Opcode::ReleaseLock:
            {
                uint32_t lockNum = registers[arg1];
                if (lockNum < locks.size() && locks[lockNum] == registers[12])
                {
                    locks[lockNum] = -1;
                    std::cout << "[LOCK] Process " << registers[12] << " released lock " << lockNum << std::endl;
                }
                break;
            }
        case Opcode::ReleaseLockI:
            {
                if (arg1 < locks.size() && locks[arg1] == registers[12])
                {
                    locks[arg1] = -1;
                    std::cout << "[LOCK] Process " << registers[12] << " released lock " << arg1 << std::endl;
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
           
        default:
            std::cerr << "Unknown opcode: " << static_cast<int>(opcodeByte) << std::endl;
            break;
    }

    if (shouldIncrementIP) {
        registers[11] = ip + 9;
    }

}

void CPU::run() {
    
    std::cout << "[DEBUG] IP: " << std::hex << registers[11] << std::endl;
    while (true) 
    {
        executeNextInstruction();

        if (sleepRequested)
        {
            return;
        }
        
        if (terminated) {
            std::cout << "[DEBUG] CPU termination flag set, exit run()\n";
            return;
        }
        
        
    }
}





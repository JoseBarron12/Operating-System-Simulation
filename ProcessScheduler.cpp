#include "ProcessScheduler.h"

void ProcessScheduler::addProcess(PCB* process) {
    readyQueue.push(process);
}

PCB* ProcessScheduler::getNextProcess() 
{
    while (!readyQueue.empty()) 
    {
        PCB* next = readyQueue.top();
        readyQueue.pop();

        if (next->state != "Terminated" || next->state == "Waiting") 
        {
            runningProcess = next;
            next->state = "Running";
            std::cout << "[SCHEDULER] Process " << next->processId 
                      << " is now running (Priority: " << next->priority << ")\n";
            return next;
        }
    }

    runningProcess = nullptr;
    return nullptr;
}


void ProcessScheduler::switchProcess(uint32_t* cpuRegisters, bool& signFlag, bool& zeroFlag) 
{
    if (runningProcess) 
    {
        runningProcess->saveState(cpuRegisters, signFlag, zeroFlag);
        runningProcess->contextSwitches++;

        if (runningProcess->state != "Terminated") 
        {
            runningProcess->state = "Ready";
            readyQueue.push(runningProcess);
        }
    }

    if (readyQueue.empty()) 
    {
        std::cout << "[SCHEDULER] No more processes to schedule.\n";
        runningProcess = nullptr;
        return;
    }

    runningProcess = getNextProcess();
    runningProcess->restoreState(cpuRegisters, signFlag, zeroFlag);
    //std::cout << "[DEBUG] Restored IP: " << std::hex << cpuRegisters[11] << std::endl;
    runningProcess->state = "Running";

    std::cout << "[SCHEDULER] Switching to Process " << runningProcess->processId 
              << " (Priority: " << runningProcess->priority << ")\n";
}


PCB* ProcessScheduler::getRunningProcess() 
{
    return runningProcess;
}

PCB* ProcessScheduler::getProcessByPid(uint32_t pid) {
    if (runningProcess && runningProcess->processId == pid) return runningProcess;
    
    std::priority_queue<PCB*, std::vector<PCB*>, ComparePriority> tempQueue = readyQueue;
    while (!tempQueue.empty()) {
        PCB* p = tempQueue.top();
        tempQueue.pop();
        if (p->processId == pid) return p;
    }
    return nullptr;
}


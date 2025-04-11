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

        if (next->state != "Terminated") 
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
    std::cout << "[DEBUG] Restored IP: " << std::hex << cpuRegisters[11] << std::endl;
    runningProcess->state = "Running";

    std::cout << "[SCHEDULER] Switching to Process " << runningProcess->processId 
              << " (Priority: " << runningProcess->priority << ")\n";
}


PCB* ProcessScheduler::getRunningProcess() 
{
    return runningProcess;
}
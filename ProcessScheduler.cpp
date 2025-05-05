// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "ProcessScheduler.h"

/**
 * Adds a process to the ready queue for scheduling.
 * 
 * @param process Pointer to the PCB of the process to be scheduled
 */
void ProcessScheduler::addProcess(PCB* process) {
    readyQueue.push(process);
}

/**
 * Retrieves the next process to run from the ready queue.
 * 
 * 
 * @return Pointer to the next PCB to run, or nullptr if no valid process is found
 */
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

/**
 * Saves the current running process state and switches to the next ready process.
 * 
 * @param cpuRegisters Current CPU register state to be saved
 * @param signFlag     Sign flag to save and restore
 * @param zeroFlag     Zero flag to save and restore
 */
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

/**
 * Returns a pointer to the currently running process.
 * 
 * @return Pointer to the currently running PCB, or nullptr if none is running
 */
PCB* ProcessScheduler::getRunningProcess() 
{
    return runningProcess;
}

/**
 * Searches the running and ready processes for one with the specified PID.
 * 
 * @param pid Process ID to search for
 * @return Pointer to the matching PCB, or nullptr if not found
 */
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


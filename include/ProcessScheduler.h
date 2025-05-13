// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef PROCESS_SCHEDULER_H
#define PROCESS_SCHEDULER_H

#include <queue>
#include "pcb.h"

/**
 * Comparison functor to order processes in the priority queue.
 */
struct ComparePriority 
{
    bool operator()(PCB* p1, PCB* p2)
    {
        return p1->priority < p2->priority; 
    }
};

/**
 * @class ProcessScheduler
 * @brief Manages the scheduling of processes using a priority-based ready queue.
 *
 */
class ProcessScheduler 
{
private:
    std::priority_queue<PCB*, std::vector<PCB*>, ComparePriority> readyQueue; // Queue of ready processes ordered by priority
    PCB* runningProcess; // Pointer to the currently running process
public:
    /// @brief Methods Handling Processing logic
    void addProcess(PCB* process);
    void switchProcess(uint32_t* cpuRegisters, bool& signFlag, bool& zeroFlag);
    
    /// @brief Helper methods
    PCB* getNextProcess();
    PCB* getRunningProcess();
    PCB* getProcessByPid(uint32_t pid);
};

#endif // PROCESS_SCHEDULER_H

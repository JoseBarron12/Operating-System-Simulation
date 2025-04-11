#ifndef PROCESS_SCHEDULER_H
#define PROCESS_SCHEDULER_H

#include <queue>
#include "pcb.h"

struct ComparePriority 
{
    bool operator()(PCB* p1, PCB* p2)
    {
        return p1->priority < p2->priority; 
    }
};


class ProcessScheduler 
{
private:
std::priority_queue<PCB*, std::vector<PCB*>, ComparePriority> readyQueue;
    PCB* runningProcess;
public:
    void addProcess(PCB* process);
    PCB* getNextProcess();
    void switchProcess(uint32_t* cpuRegisters, bool& signFlag, bool& zeroFlag);
    PCB* getRunningProcess();
};

#endif // PROCESS_SCHEDULER_H

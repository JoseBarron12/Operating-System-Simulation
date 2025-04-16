#ifndef CPU_H_INC
#define CPU_H_INC

#include <stdint.h>
#include <unistd.h>
#include <iomanip>
#include <array>
#include <vector>
#include "Opcode.h"
#include<cstdint>
#include "memory.h"
#include "ProcessScheduler.h"
#include <functional>

class OperatingSystem; 

class CPU 
{
private: 
    std::vector<Opcode> opcodes; // Opcodes
    u_int32_t registers[15]; // 14 registers
    
    uint64_t systemClock;         // Tick counter for executed instruction
                                  // Interrupt function pointers
    
    ProcessScheduler& scheduler;
    memory& mem;
    
    bool signFlag;                // Flag for comparisons
    bool zeroFlag;                // Flag for equality checks

    bool sleepRequested = false;  
    uint32_t sleepDuration = 0;   
    bool terminated = false; 
    bool quantumExpired = false;

    std::vector<int32_t> locks;
    std::array<std::vector<PCB*>, 11> lockWaitQueues; 
    bool waitingOnLock = false;


    std::array<bool, 10> eventStates;                   
    std::array<std::vector<PCB*>, 10> eventWaitQueues;

    OperatingSystem* os;

    bool preemptNow = false;

    bool waitingOnEvent = false;


public:
    CPU(memory& memRef, ProcessScheduler& sched);
    void loadProgram(std::vector<uint8_t>& program);
    void releaseAllHeldLocks(uint32_t pid);
    void executeNextInstruction();    // Runs one instruction
    void run();

    int32_t getRegister(int index) const { return registers[index]; }
    void setRegister(int index, int32_t value) { registers[index] = value; }
    
    uint64_t getSystemClock() const { return systemClock; }
    void setSystemClock(uint64_t value) { systemClock = value; }
    
    uint32_t getInstructionPointer() const { return registers[11]; }
    void setInstructionPointer(uint32_t value) { registers[11] = value; }
    
    bool getSignFlag() const { return signFlag; }
    void setSignFlag(bool value) { signFlag = value; }
    
    bool getZeroFlag() const { return zeroFlag; }
    void setZeroFlag(bool value) { zeroFlag = value; }

    bool isSleepRequested() const { return sleepRequested; }  
    uint32_t getSleepDuration() const { return sleepDuration; }  
    void clearSleepRequest() { sleepRequested = false; sleepDuration = 0; } 
    void setSleepDuration( uint32_t sleep) { sleepDuration = 0; }
    
    bool isTerminated() const { return terminated; }  
    void clearTerminatedFlag() { terminated = false; }

    bool isExpired() const { return quantumExpired; }
    void clearExpiredFlag() { terminated = false; }

    u_int32_t* getRegisters() { return registers; }

    std::function<void()> onCycleCallback;
   
    bool isWaitingOnLock() const { return waitingOnLock; }
    void setWaitingOnLock(bool value) { waitingOnLock = value; }
    void clearWaitingOnLock() { waitingOnLock = false; }

    void setOSPointer(OperatingSystem* os) { this->os = os; }

    void triggerPreemption() { preemptNow = true; }
    void clearPreemption() { preemptNow = false; }
    bool isPreempting() const { return preemptNow; }

    void requestEventWait() { waitingOnEvent = true; }
    bool isWaitingOnEvent() const { return waitingOnEvent; }
    void clearWaitingOnEvent() { waitingOnEvent = false; }


};


#endif
// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

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

/**
 * @class CPU
 * @brief Simulates the execution of instructions for processes using a virtual CPU.
 *        Handles memory interaction, process scheduling, synchronization, and control flow.
 */
class CPU 
{
private: 
    std::vector<Opcode> opcodes; // Opcodes
    u_int32_t registers[15]; // 14 registers
    
    uint64_t systemClock;         // Tick counter for executed instruction
                                  // Interrupt function pointers
    
    ProcessScheduler& scheduler;  // Reference to systems scheduler
    memory& mem;                  // Reference to systems memory
    
    bool signFlag;                // Flag for comparisons
    bool zeroFlag;                // Flag for equality checks

    bool sleepRequested = false;  // Sleep flag
    uint32_t sleepDuration = 0;   // Remaining sleep duration
    bool terminated = false;      // Termination flag
    bool quantumExpired = false;  // Quantum expiration flag

    std::vector<int32_t> locks;   // Lock table
    std::array<std::vector<PCB*>, 11> lockWaitQueues;   // Queue for waiting process for lock
    bool waitingOnLock = false;   // Lock flag


    std::array<bool, 10> eventStates;                   // Event table
    std::array<std::vector<PCB*>, 10> eventWaitQueues;  // Queue for waiting process for events
    bool waitingOnEvent = false;                        // Event Flag
    
    OperatingSystem* os;          // Pointer to operating system

    bool preemptNow = false;      // Preemption flag
    
    bool blockedOnMemory = false; // Blocked memory flag


public:
    /**
     * @brief Constructor initializes CPU state, registers, and memory/scheduler references.
     * @param memRef Reference to memory
     * @param sched Reference to the process scheduler
     */
    CPU(memory& memRef, ProcessScheduler& sched);
    
    /**
     * @brief Loads a program into memory.
     * @param program program to be loaded.
     */
    void loadProgram(std::vector<uint8_t>& program);
    
    /**
     * @brief Releases all locks held by the specified process.
     * @param pid Process ID whose locks are to be released
     */
    void releaseAllHeldLocks(uint32_t pid);

    /**
     * @brief Executes the next instruction of the currently running process.
     */
    void executeNextInstruction();    
    
    /**
     * @brief Executes instructions in a loop until interrupted or terminated.
     */
    void run(); 

    /** 
     * @brief Helper methods used
     */
    
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

    bool isBlocked() const { return blockedOnMemory;}
    void clearBlocked() {blockedOnMemory = false;}
    
};

#endif
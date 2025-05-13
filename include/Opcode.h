// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef OPCODE_H
#define OPCODE_H

/**
 * Enum representing the opcode set supported by the virtual CPU.
 * Each opcode corresponds to a specific machine instruction.
 */
enum Opcode {
    Incr = 0x01,
    Addi = 0x02,
    Addr = 0x03,
    Pushr = 0x04,
    Pushi = 0x05,
    Movi = 0x06,
    Movr = 0x07,
    Movmr = 0x08,
    Movrm = 0x09,
    Movmm = 0x0A,
    Printr = 0x0B,
    Printm = 0x0C,
    Printcr = 0x0D,
    Printcm = 0x0E,
    Jmp = 0x0F,
    Jmpi = 0x10,
    Jmpa = 0x11,
    Cmpi = 0x12,
    Cmpr = 0x13,
    Jlt = 0x14,
    Jlti = 0x15,
    Jlta = 0x16,
    Jgt = 0x17,
    Jgti = 0x18,
    Jgta = 0x19,
    Je = 0x1A,
    Jei = 0x1B,
    Jea = 0x1C,
    Call = 0x1D,
    Callm = 0x1E,
    Ret = 0x1F,
    Exit = 0x20,
    Popr = 0x21,
    Popm = 0x22,
    Sleep = 0x23,
    Input = 0x24,
    Inputc = 0x25,
    SetPriority = 0x26,
    SetPriorityI = 0x27,
    MapSharedMem = 0x28,
    AcquireLock = 0x29,
    AcquireLockI = 0x2A,
    ReleaseLock = 0x2B,
    ReleaseLockI = 0x2C,
    SignalEvent = 0x2D,
    SignalEventI = 0x2E,
    WaitEvent = 0x2F,
    WaitEventI = 0x30,
    Alloc = 0x31,
    FreeMemory = 0x32,
    TerminateProcess = 0x33
};


#endif 
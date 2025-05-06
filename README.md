# Operating System Simulation [MidOS]- CSCI 480 PE1  
**Author:** Jose Barron  
**Z-ID:** Z2013735  

## Completed Modules

### ✅ Module 1 – The Virtual Machine
- Implements a virtual CPU with general-purpose and special-purpose registers (e.g., IP, PID, SP).
- Executes a custom instruction set with support for arithmetic, stack, and control operations.
- Tracks clock cycles and system flags (zero, sign).
- Supports loading and executing programs independently.

### ✅ Module 2 – Memory Management
- Implements a virtual memory system using paged memory.
- Maps virtual addresses to physical addresses using a page table.
- Supports memory isolation and simulates MMU behavior.
- Loads programs into memory non-contiguously.

### ✅ Module 3 – Process Management
- Implements PCBs (Process Control Blocks).
- Maintains process state (Ready, Running, Sleeping, Waiting, Terminated).
- Supports preemptive priority-based scheduling and context switching.
- Tracks clock cycles, context switches, and sleep duration.

### ✅ Module 4 – Shared memory
- Adds support for:
  - Shared Memory (`MapSharedMem`)
  - Lock acquisition and release (`AcquireLock`, `ReleaseLock`)
  - Event signaling and waiting (`SignalEvent`, `WaitEvent`)
- Processes waiting on locks or events are properly queued and rescheduled.

### ✅ Module 5 – Heap Allocation 
- Adds heap region per process for dynamic memory allocation.
- Implements `Alloc` and `FreeMemory` opcodes to manage heap segments.
- Maintains a heap page table to track allocations and prevent overflows.
- Heap segments include address, size, and usage status.

### ✅ Module 6 – Virtual Memory
- Implements a virtual memory system:
  - Virtual-to-physical page mapping.
  - `isValid` and `isDirty` flags per page.
  - Least Recently Used (LRU) page replacement.
- Handles page faults and swaps pages to/from simulated disk.
- Tracks page faults, swap-ins, and swap-outs.
- Blocks processes that can’t proceed due to lack of memory and resumes them when possible.

---

## Additional Features (Overview.pdf Compliance)

- ✅ **Idle Process**
  - Defined in `idle.txt` with PID 999, runs only when no other process is eligible.
  - Always has the lowest priority, runs in fixed memory location.

- ✅ **Shared Memory**
  - 10 fixed shared memory regions (1000 bytes each).
  - Support for `MapSharedMem` to enable inter-process communication.

- ✅ **Full Virtual Memory Model**
  - Every process uses virtual addresses exclusively.
  - Page tables created at load time, all memory accesses validated.
  - Page faults suspend the current process if no memory is available.

- ✅ **Memory Cleanup**
  - On process exit, all physical pages and heap allocations are freed.
  - Locks held by exiting process are released and waiting processes are rescheduled.

- ✅ **Priority Inversion Handling**
  - Lower-priority processes inherit the priority of higher-priority blocked processes temporarily.
  - Reverts after lock is released.

- ✅ **Console I/O**
  - `Printr`, `Printm`, `Printcr`, `Printcm` and `Input`, `Inputc` implemented for console I/O.

- ✅ **Stack Overflow Prevention**
  - Stack region is isolated and protected from overlapping with heap or code.

- ✅ **Instruction Execution & Safety**
  - Robust execution of all opcodes with register validation and fault checks.
  - Illegal memory access results in process termination and full diagnostic printout.

## How to Compile
1. Compile the project using `g++`:
   ```bash
   g++ -o os main.cpp cpu.cpp memory.cpp ProcessScheduler.cpp pcb.cpp program.cpp OperatingSystem.cpp hex.cpp -std=c++17

## How to Run
```bash
./os <memory_size_in_bytes> <program1.txt> <program2.txt> ... <idle.txt>
```
- `./os` is the compiled executable for MidOS.
- The first argument is the **total program memory size** in bytes (e.g., 32768).
- Each `<programN.txt>` is a user program loaded in the order they appear.
- Programs **earlier on the command line are given higher priority**.
- A **maximum of 31 user programs** can be provided.
- EXAMPLE 1: ./os 1024 test_opcodes.txt legal.txt illegal.txt killer.txt high_waiter.txt lock_holder.txt low_waiter.txt heapfail.txt heap_test.txt sleep1.txt sleep2.txt sleep3.txt test_eventi.txt test_eventi_signal.txt sharedmem1.txt sharedmem2.txt SetPriorityI.txt SetPriority.txt SetPriorityWrong.txt
- EXAMPLE 2: ./os 512 sleep1.txt sleep2.txt sleep3.txt


[![Open in Codespaces](https://classroom.github.com/assets/launch-codespace-2972f46106e565e64193e422d61a12cf1da4916b45550586e14ef0a7c637dd04.svg)](https://classroom.github.com/open-in-codespaces?assignment_repo_id=18102137)

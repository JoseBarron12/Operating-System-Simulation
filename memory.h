// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#ifndef MEMORY_H_INC
#define MEMORY_H_INC

#include <stdint.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include "hex.h"
#include <unordered_set>

#define PAGE_SIZE 256 // Size of a memory page in bytes

class OperatingSystem; 

/**
 * @brief Describes a virtual-to-physical page mapping entry.
 */
struct PageEntry 
{
    uint32_t physicalPage;  // Physical page number
    bool isValid;           // Valid flag
    bool isDirty;           // Dirty flag
    uint32_t pid;           // Process id
    uint64_t lastUsedTime;  // Timestamp for LRU 
};

/**
 * @brief Represents a shared memory region.
 */
struct SharedRegion 
{
    uint32_t physicalPage;  // Physical page backing the shared region
    bool allocated;         // Allocation flag 
};

/**
 * @class memory
 * @brief The memory class simulates a paged memory system with support for virtual memory,
 * paging, swapping, and shared memory.
 */
class memory : public hex
{
public :
    /**
     * @brief Constructs a memory instance of specified size.
     * @param siz The total size of simulated memory in bytes.
     */
    memory(uint32_t siz );
    
    // Methods to cleans up memory structures
    ~memory();


    // Checks if an address is outside the valid memory range
    bool check_illegal(uint32_t addr) const;
    
    // Returns the total memory size
    uint32_t get_size() const;
    void deallocateProcessPages(uint32_t pid);
    
    // Methods to read memory
    uint8_t get8(uint32_t addr, uint32_t pid);
    uint16_t get16(uint32_t addr, uint32_t pid);
    uint32_t get32(uint32_t addr, uint32_t pid);
    uint32_t getaddress(uint32_t addr, uint32_t pid);
    uint32_t getpages();
    uint32_t getFreePageCount() const;
    bool hasFreePage() const;
    std::unordered_map<uint32_t, PageEntry> getPagingTable() const;
    
    // Methods to write to memory
    void set8 (uint32_t addr, uint8_t val);
    void set16 (uint32_t addr, uint16_t val);
    void set32 (uint32_t addr, uint32_t val);

    // Methods to handle memory allocation
    uint32_t allocatePage();
    void mapPage(uint32_t startAddress, uint32_t physicalPage, uint32_t pid);
    bool pageExists(uint32_t startAddress) const; 
    
    // debug:
    void printPagingTable() const;
    void debugDump() const;
    void printMemoryMetrics() const;
    void debugPrintFreePages() const;

    // Methods to handle LRU
    uint32_t findLRUPage(); 
    void swapOutPage(uint32_t virtualAddr);
    
    // Sets a pointer to the OperatingSystem object
    void setOSPointer(OperatingSystem* os) { this->os = os; }
    
    // Table mapping shared memory region IDs to physical pages
    std::unordered_map<uint32_t, SharedRegion> sharedMemoryTable;


private :
    std::vector<uint8_t> mem;                                      // The physical memory byte array
    std::unordered_map<uint32_t, PageEntry> paging_table;          // Virtual to physical mapping
    uint32_t pages;                                                // Total number of pages
    std::unordered_map<uint32_t, std::vector<uint8_t>> swapSpace;  // Swap disk space
    uint32_t nextFreePage;                                         // Next available page
    std::unordered_set<uint32_t> freePages;                        // Set of free pages
    uint32_t pageFaultCount = 0;                                   // Total number of page fault
    uint32_t pagesSwappedOut = 0;                                  // Pages swapped out to disk
    uint32_t pagesSwappedIn = 0;                                   // Pages swapped in from disk
    OperatingSystem* os;                                           // Pointer to the operation system
    
};    

#endif
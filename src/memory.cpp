// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "memory.h"
#include <stdint.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include "hex.h"
#include <cmath>
#include <string>
#include <fstream> 
#include <unordered_set>
#include "ProcessScheduler.h"
#include "OperatingSystem.h" 

static uint64_t systemClock = 0;

/**
 * Constructs a memory object with the specified size, initializes physical memory,
 * divides it into pages, creates shared memory regions, and reserves space
 * for the idle process with PID 999.
 *
 * @param siz The total size of memory to allocate (rounded to the nearest multiple of 16).
 */
memory::memory(uint32_t siz )
{
    // Memory region of physical memory for programs
    siz = (siz+15)&0xfffffff0;
    pages = (siz + PAGE_SIZE - 1) / PAGE_SIZE;
    mem.resize(pages * PAGE_SIZE, 0x00);

    for (uint32_t i = 0; i < pages; ++i)
    {
        freePages.insert(i);
    }  
    
    // Shared memory region of physical memory
    uint32_t sharedMemStart = mem.size();
    mem.resize(mem.size() + (1000 * 10), 0x00);
    for (int i = 0; i < 10; ++i) 
    {
        uint32_t baseAddr = sharedMemStart + (i * 1000);
        sharedMemoryTable[i] = { baseAddr, true }; 
        std::cout << "[SHARED INIT] Region " << i << " starts at physical address 0x" 
                << std::hex << baseAddr << std::dec << "\n";
    }

    // Memory region of physical memory for idle process
    uint32_t idleStart = mem.size(); 
    if (idleStart % PAGE_SIZE != 0) 
    {
        idleStart = ((idleStart / PAGE_SIZE) + 1) * PAGE_SIZE;
        mem.resize(idleStart, 0x00); // pad to alignment
    }

    // Reserve 1 full page for idle
    mem.resize(idleStart + PAGE_SIZE, 0x00);

    uint32_t idlePhysPage = idleStart / PAGE_SIZE;
    paging_table[0xF000] = {
        .physicalPage = idlePhysPage,
        .isValid = true,
        .isDirty = false,
        .pid = 999,
        .lastUsedTime = 0
    };

    std::cout << "[IDLE ALLOCATION] Process 999 fixed at physical 0x" 
            << std::hex << (idlePhysPage * PAGE_SIZE) << std::dec << "\n";


}

/**
 * Destructor for the memory class.
 * Clears all allocated memory and frees physical pages.
 */
memory::~memory()
{
    mem.clear();
    freePages.clear();
}

/**
 * Checks whether the given address is within valid memory bounds.
 *
 * @param addr The address to validate.
 * @return True if the address is illegal (out of bounds), false otherwise.
 */
bool memory::check_illegal(uint32_t addr) const
{
    if (addr >= mem.size()) 
    {
        std::string hex = hex::to_hex0x32(addr);
        std::cout << "WARNING: Address out of range: " << hex << '\n';
        return true;
    }
    return false;
}

/**
 * Returns the size of the physical memory in bytes
 * 
 * @return Memory size in bytes.
 */
uint32_t memory::get_size() const
{
    return mem.size();
}

/**
 * Reads a byte from memory using a process's virtual address.
 */
uint8_t memory::get8(uint32_t addr, uint32_t pid)
{
    uint32_t physical_addr = getaddress(addr, pid);
    return mem[physical_addr];
}

/**
 * Reads a 16-bit value from memory using virtual address.
 */
uint16_t memory::get16(uint32_t addr, uint32_t pid)
{
    return (get8(addr + 1, pid) << 8) | get8(addr, pid);
}

/**
 * Reads a 32-bit value from memory using virtual address.
 */
uint32_t memory::get32(uint32_t addr, uint32_t pid)
{
    return (get16(addr + 2, pid) << 16) | get16(addr, pid);
}

/**
 * Translates a virtual address to a physical address for a specific process.
 * Handles page faults, page allocation, and swapping if needed.
 *
 * @param virtualAddr The virtual address being accessed.
 * @param pid The process ID performing the access.
 * @return The corresponding physical address in memory.
 * @throws runtime_error If a page fault blocks the process or an illegal access occurs.
 */

uint32_t memory::getaddress(uint32_t virtualAddr, uint32_t pid)
{
    uint32_t start_address = virtualAddr & ~(PAGE_SIZE - 1); 
    uint32_t offset = virtualAddr & (PAGE_SIZE - 1);

    if (!pageExists(start_address)) 
    {
        if (pid == 999) 
        {
            std::cerr << "[IDLE PROTECT] Page 0x" << std::hex << start_address << " missing for idle PID 999 — aborting.\n";
            exit(1); // this should never happen
        }
        
        if (!hasFreePage()) 
        {
            if (os && os->hasSleepingOrWaitingProcesses() && !os->hasAllSleepingOrWaitingProcesses() && pid != 999) 
            {
                std::cout << "[MEMORY BLOCK] No pages available. Process " << pid << " will wait for memory.\n";
                throw std::runtime_error("BLOCKED_PAGE_FAULT");
            }

            uint32_t lru = findLRUPage();
            if (lru == UINT32_MAX) 
            {
                std::cerr << "[FATAL] No memory available and no LRU page to evict.\n";
                exit(1);
            }
            swapOutPage(lru);
        }


        uint32_t newPhysicalPage = allocatePage();
        if (newPhysicalPage == UINT32_MAX)
        {
            std::cerr << "[FATAL] Allocation failed after swap out.\n";
            exit(1);
        }

        mapPage(start_address, newPhysicalPage, pid);
    }

    PageEntry& entry = paging_table[start_address];

    if (entry.pid != pid && entry.pid != static_cast<uint32_t>(-1)) 
    {
        std::cerr << "[ILLEGAL ACCESS] Process " << pid 
                << " attempted access to virtual address 0x" << std::hex << virtualAddr 
                << " owned by PID " << entry.pid << std::dec << "\n";

        throw std::runtime_error("Illegal memory access by process");
    }
    
    if (!entry.isValid) 
    {
        if (pid == 999) 
        {
            std::cerr << "[IDLE PROTECT] Page 0x" << std::hex << start_address << " missing for idle PID 999 — aborting.\n";
            exit(1); // this should never happen
        }
       
        std::cout << "[PAGE FAULT] Page 0x" << std::hex << start_address << " not in memory.\n";
        pageFaultCount++; 

        if (!hasFreePage()) 
        {
            if (os && os->hasSleepingOrWaitingProcesses() && !os->hasAllSleepingOrWaitingProcesses()) 
            {
                std::cout << "[MEMORY BLOCK] No pages available. Process " << pid << " will wait for memory.\n";
                throw std::runtime_error("BLOCKED_PAGE_FAULT");
            }

            uint32_t lru = findLRUPage();
            if (lru == UINT32_MAX) 
            {
                std::cerr << "[FATAL] No memory available and no LRU page to evict.\n";
                exit(1);
            }
            swapOutPage(lru);
        }


        uint32_t physicalPage = allocatePage();
        if (physicalPage == UINT32_MAX)
        {
            std::cerr << "[FATAL] Allocation failed after swap out.\n";
            exit(1);
        }

        entry.physicalPage = physicalPage;
        entry.isValid = true;
        entry.isDirty = false;

        if (swapSpace.count(start_address)) 
        {
            pagesSwappedIn++; 
            const std::vector<uint8_t>& data = swapSpace[start_address];
            for (uint32_t i = 0; i < PAGE_SIZE; ++i)
            {
                uint32_t physAddr = (physicalPage * PAGE_SIZE) + i;
                mem[physAddr] = data[i];
            }
            std::cout << "[SWAP-IN] Loaded page 0x" << std::hex << start_address << " from disk.\n";
        } 
        else 
        {
            std::cout << "[SWAP-IN] No saved page on disk for 0x" << std::hex << start_address << ", zero-filled.\n";
        }
    }

    entry.lastUsedTime = ++systemClock;

    return (entry.physicalPage * PAGE_SIZE) + offset;
}
/**
 * Returns number of memory pages.
 */
uint32_t memory::getpages()
{
    return pages;
}

/**
 * Writes a byte to memory.
 */
void memory::set8(uint32_t addr, uint8_t val)
{
    if (!check_illegal(addr))
    {
        mem[addr] = val;  
        uint32_t virtual_page = addr & ~(PAGE_SIZE - 1);
        if (paging_table.count(virtual_page)) 
        {
            paging_table[virtual_page].isDirty = true;
        }

    }
}

/**
 * Writes a 16-bit value to memory.
 */
void memory::set16(uint32_t addr, uint16_t val)
{
    set8(addr, val & 0xFF);
    set8(addr + 1, val >> 8);
}

/**
 * Writes a 32-bit value to memory.
 */
void memory::set32(uint32_t addr, uint32_t val)
{
    set16(addr, val & 0xFFFF);
    set16(addr + 2, val >> 16);
}

/**
 * Maps a virtual page to a physical page for a given process in the paging table.
 * Special handling prevents remapping the idle process's page.
 *
 * @param startAddress The start address of the virtual page.
 * @param physicalPage The index of the physical page to map.
 * @param pid The process ID owning the page.
 */
void memory::mapPage(uint32_t startAddress, uint32_t physicalPage, uint32_t pid) 
{
    if (pid == 999 && paging_table.count(startAddress)) 
    {
        std::cout << "[MAP] Skipped remapping idle page at 0x" 
                  << std::hex << startAddress << std::dec << "\n";
        return;
    }
    
    PageEntry entry;
    entry.physicalPage = physicalPage;
    entry.isValid = false; // don't mark valid until loaded
    entry.isDirty = false;
    entry.pid = pid; 
    entry.lastUsedTime = 0; 
    paging_table[startAddress] = entry;

    std::cout << "[MAP] Mapped virtual page 0x" << std::hex << startAddress
              << " → physical page " << physicalPage << std::dec << std::endl;
}

/**
 * Checks if a virtual page has already been mapped.
 */
bool memory::pageExists(uint32_t startAddress) const 
{
    return paging_table.find(startAddress) != paging_table.end();
}

/**
 * Allocates a free physical memory page from the available pool.
 *
 * @return The physical page number that was allocated.
 *         Returns UINT32_MAX if no pages are available.
 */


uint32_t memory::allocatePage() 
{
    if (freePages.empty()) 
    {
        std::cerr << "[ALLOCATE ERROR] No free physical pages available!\n";
        return UINT32_MAX;
    }

    //debugPrintFreePages();
    
    uint32_t page = *freePages.begin();
    freePages.erase(page);

    std::cout << "[ALLOCATE] Allocated physical page " << page << "\n";
    return page;
}

/**
 * Prints the current paging table to the console.
 */
void memory::printPagingTable() const 
{
    std::cout << "===== Paging Table =====" << std::endl;
    for (const auto& entry : paging_table) 
    {
        uint32_t startAddress = entry.first;
        const PageEntry& page = entry.second;

        std::cout << "Start Address: 0x" << std::hex << startAddress
                  << " | Physical Page: " << page.physicalPage
                  << " | IsValid: " << std::boolalpha << page.isValid
                  << " | IsDirty: " << std::boolalpha << page.isDirty
                  << " | PID: " << std::dec << page.pid
                  << " | LastUsed: " << page.lastUsedTime
                  << std::endl;
    }
}

/**
 * Returns the current page table.
 */
std::unordered_map<uint32_t, PageEntry> memory::getPagingTable() const 
{
    return paging_table;  
}

/**
 * Prints raw contents of memory by physical address.
 */
void memory::debugDump() const 
{
    std::cout << "===== Memory Dump =====" << std::endl;
    for (const auto& entry : paging_table) 
    {
        uint32_t start_address = entry.first;
        const PageEntry& page = entry.second;

        std::cout << "Virtual Page Start: 0x" << std::hex << start_address
                  << " | Physical Page: " << page.physicalPage
                  << " | IsValid: " << std::boolalpha << page.isValid
                  << " | IsDirty: " << std::boolalpha << page.isDirty
                  << " | PID: " << std::dec << page.pid
                  << " | LastUsed: " << page.lastUsedTime
                  << std::endl;

        for (uint32_t offset = 0; offset < PAGE_SIZE; ++offset) 
        {
            uint32_t physical_addr = (page.physicalPage * PAGE_SIZE) + offset;
            std::cout << "  [DEBUG] Memory[" << physical_addr << "] = " 
                      << std::hex << static_cast<int>(mem[physical_addr]) << std::endl;
        }
    }
}

/**
 * Checks whether memory has any free pages.
 */
bool memory::hasFreePage() const 
{
    return !freePages.empty();
}

/**
 * Finds the least recently used page.
 */
uint32_t memory::findLRUPage()
{
    uint64_t minTime = UINT64_MAX;
    uint32_t lruPage = UINT32_MAX;

    for (const auto& [virtAddr, entry] : paging_table) 
    {
        if (entry.isValid && entry.lastUsedTime < minTime && entry.pid != 999) 
        {
            minTime = entry.lastUsedTime;
            lruPage = virtAddr;
        }
    }
    return lruPage;
}

/**
 * Swaps a virtual page out of memory. If the page is dirty, writes it to swap space.
 *
 * @param virtualAddr The virtual address of the page to swap out.
 */
void memory::swapOutPage(uint32_t virtualAddr) 
{
    if (!paging_table.count(virtualAddr)) return;

    PageEntry& entry = paging_table[virtualAddr];
    if (!entry.isValid) return;

    pagesSwappedOut++; 

    if (entry.isDirty) 
    {
        std::vector<uint8_t> pageData;
        for (uint32_t i = 0; i < PAGE_SIZE; ++i) 
        {
            uint32_t physAddr = (entry.physicalPage * PAGE_SIZE) + i;
            pageData.push_back(mem[physAddr]);
        }

        swapSpace[virtualAddr] = pageData;
        std::cout << "[SWAP] Page 0x" << std::hex << virtualAddr << " written to disk.\n";
    } 
    else 
    {
        std::cout << "[SWAP] Page 0x" << std::hex << virtualAddr << " marked clean (no disk write needed).\n";
    }

    freePages.insert(entry.physicalPage); 
    entry.isValid = false;
}

/**
 * Returns the count of free physical pages.
 */
uint32_t memory::getFreePageCount() const 
{
    return freePages.size();
}

/**
 * Prints memory usage metrics such as faults and swap stats.
 */
void memory::printMemoryMetrics() const 
{
    std::cout << "\n=== Memory Metrics Summary ===\n";
    std::cout << "Total Page Faults: " << pageFaultCount << '\n';
    std::cout << "Pages Swapped Out: " << pagesSwappedOut << '\n';
    std::cout << "Pages Swapped In:  " << pagesSwappedIn << '\n';
    std::cout << "Free Physical Pages Remaining: " << getFreePageCount() << '\n';
}


/**
 * Frees all memory pages owned by the given process and removes them from the paging table.
 *
 * @param pid The process ID whose pages should be deallocated.
 */
void memory::deallocateProcessPages(uint32_t pid)
{
    for (auto it = paging_table.begin(); it != paging_table.end(); )
    {
        if (it->second.pid == pid)
        {
            uint32_t physPage = it->second.physicalPage;

            // Don't double-free shared or idle pages
            if (physPage != UINT32_MAX && freePages.count(physPage) == 0)
            {
                freePages.insert(physPage);
            }

            it = paging_table.erase(it); // Remove the page mapping
        }
        else
        {
            ++it;
        }
    }
}

/**
 * Outputs free page list for debugging.
 */
void memory::debugPrintFreePages() const
{
    std::cout << "[Memory Deallocation] Free Physical Pages After Deallocation: ";
    for (uint32_t page : freePages)
    {
        std::cout << page << " ";
    }
    std::cout << std::endl;
}

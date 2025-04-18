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

memory::memory(uint32_t siz )
{
    siz = (siz+15)&0xfffffff0;
    pages = (siz + PAGE_SIZE - 1) / PAGE_SIZE;
    mem.resize(pages * PAGE_SIZE, 0x00);

    for (uint32_t i = 0; i < pages; ++i)
    {
        freePages.insert(i);
    }  
    
    uint32_t sharedMemStart = mem.size();
    mem.resize(mem.size() + (1000 * 10), 0x00);
    for (int i = 0; i < 10; ++i) 
    {
        uint32_t baseAddr = sharedMemStart + (i * 1000);
        sharedMemoryTable[i] = { baseAddr, true }; 
        std::cout << "[SHARED INIT] Region " << i << " starts at physical address 0x" 
                << std::hex << baseAddr << std::dec << "\n";
    }

    // Align to next page boundary
    uint32_t idleStart = mem.size();
    if (idleStart % PAGE_SIZE != 0) {
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

memory::~memory()
{
    mem.clear();
    freePages.clear();
}

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

uint32_t memory::get_size() const
{
    return mem.size();
}

uint8_t memory::get8(uint32_t addr, uint32_t pid)
{
    uint32_t physical_addr = getaddress(addr, pid);
    return mem[physical_addr];
}

uint16_t memory::get16(uint32_t addr, uint32_t pid)
{
    return (get8(addr + 1, pid) << 8) | get8(addr, pid);
}

uint32_t memory::get32(uint32_t addr, uint32_t pid)
{
    return (get16(addr + 2, pid) << 16) | get16(addr, pid);
}


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


uint32_t memory::getpages()
{
    return pages;
}

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

void memory::set16(uint32_t addr, uint16_t val)
{
    set8(addr, val & 0xFF);
    set8(addr + 1, val >> 8);
}

void memory::set32(uint32_t addr, uint32_t val)
{
    set16(addr, val & 0xFFFF);
    set16(addr + 2, val >> 16);
}

bool memory::load_file(const std::string &fname )
{
    std::ifstream infile(fname, std::ios::in|std::ios::binary);

    if( !infile )
    {
        std::cerr << "\nCan't open file '" << fname << "' for reading.";
        return false;          
    }

    uint8_t i;
    infile >> std::noskipws;
    for (uint32_t addr = 0; infile >> i; ++addr)
    {
        if ( check_illegal(addr))
        {
            std::cerr << "Program too big.\n";
            infile.close();
            return false;
        }
        set8(addr,i);
    }
    infile.close();
    return true;
}

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


bool memory::pageExists(uint32_t startAddress) const 
{
    return paging_table.find(startAddress) != paging_table.end();
}

uint32_t memory::allocatePage() 
{
    if (freePages.empty()) 
    {
        std::cerr << "[ALLOCATE ERROR] No free physical pages available!\n";
        return UINT32_MAX;
    }

    debugPrintFreePages();
    
    uint32_t page = *freePages.begin();
    freePages.erase(page);

    std::cout << "[ALLOCATE] Allocated physical page " << page << "\n";
    return page;
}

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

std::unordered_map<uint32_t, PageEntry> memory::getPagingTable() const 
{
    return paging_table;  
}

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

bool memory::hasFreePage() const 
{
    return !freePages.empty();
}

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



uint32_t memory::getFreePageCount() const 
{
    return freePages.size();
}

void memory::printMemoryMetrics() const 
{
    std::cout << "\n=== Memory Metrics Summary ===\n";
    std::cout << "Total Page Faults: " << pageFaultCount << '\n';
    std::cout << "Pages Swapped Out: " << pagesSwappedOut << '\n';
    std::cout << "Pages Swapped In:  " << pagesSwappedIn << '\n';
    std::cout << "Free Physical Pages Remaining: " << getFreePageCount() << '\n';
}


void memory::deallocateProcessPages(uint32_t pid)
{
    for (auto it = paging_table.begin(); it != paging_table.end(); )
    {
        if (it->second.pid == pid)
        {
            uint32_t physPage = it->second.physicalPage;

            if (it->second.isValid)
            {
                freePages.insert(physPage); // Return physical page to pool
            }

            it = paging_table.erase(it); // Remove the page mapping
        }
        else
        {
            ++it;
        }
    }
}

void memory::debugPrintFreePages() const
{
    std::cout << "[DEBUG] Free Physical Pages After Deallocation: ";
    for (uint32_t page : freePages)
    {
        std::cout << page << " ";
    }
    std::cout << std::endl;
}

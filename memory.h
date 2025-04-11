#ifndef MEMORY_H_INC
#define MEMORY_H_INC

#include <stdint.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include "hex.h"
#include <unordered_set>

#define PAGE_SIZE 256

struct PageEntry 
{
    uint32_t physicalPage;
    bool isValid;
    bool isDirty;
    uint32_t pid;          
    uint64_t lastUsedTime; 
};


class memory : public hex
{
public :
    memory(uint32_t siz );
    ~memory();

    bool check_illegal(uint32_t addr) const;
    uint32_t get_size() const;
    uint8_t get8(uint32_t addr, uint32_t pid);
    uint16_t get16(uint32_t addr, uint32_t pid);
    uint32_t get32(uint32_t addr, uint32_t pid);
    uint32_t getaddress(uint32_t addr, uint32_t pid);
    uint32_t getpages();

    void set8 (uint32_t addr, uint8_t val);
    void set16 (uint32_t addr, uint16_t val);
    void set32 (uint32_t addr, uint32_t val);

    bool load_file(const std::string &fname );

    uint32_t allocatePage();
    void mapPage(uint32_t startAddress, uint32_t physicalPage, uint32_t pid);
    bool pageExists(uint32_t startAddress) const; 
    
    // debug:
    void printPagingTable() const;
    std::unordered_map<uint32_t, PageEntry> getPagingTable() const;
    void debugDump() const;
    void printMemoryMetrics() const;


    bool hasFreePage() const;
    uint32_t findLRUPage(); 
    void swapOutPage(uint32_t virtualAddr);
    uint32_t getFreePageCount() const;



private :
    std::vector<uint8_t> mem;
    std::unordered_map<uint32_t, PageEntry> paging_table;
    uint32_t pages;
    std::unordered_map<uint32_t, std::vector<uint8_t>> swapSpace;
    uint32_t nextFreePage;
    std::unordered_set<uint32_t> freePages;
    uint32_t pageFaultCount = 0;
    uint32_t pagesSwappedOut = 0;
    uint32_t pagesSwappedIn = 0;

};    

#endif
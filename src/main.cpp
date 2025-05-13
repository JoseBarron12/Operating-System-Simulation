// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************

#include "OperatingSystem.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) 
{
    if (argc < 3) 
    {
        std::cerr << "Usage: ./os <memory size> <program1.txt> <program2.txt> ...\n";
        return 1;
    }
    
    uint32_t memSize = std::stoi(argv[1]);
    
    int processCount = argc - 2;
    if (processCount > 31) 
    {
        std::cerr << "Error: Maximum of 31 user programs allowed.\n";
        return 1;
    }
    
    OperatingSystem os(memSize, "src/idle.txt"); // load the idle process regardless
    
    for (int i = 0; i < processCount; ++i) 
    {
        std::string filename = argv[i + 2];
        int priority = 32 - i;

        os.loadProcess(i + 1, filename, 5, 512, priority, 512);
    }
    os.cpu.setOSPointer(&os); 
    os.mem.setOSPointer(&os);
    os.start();
    return 0;
}

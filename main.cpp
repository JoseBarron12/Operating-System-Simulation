#include "OperatingSystem.h"

int main(void) 
{
    OperatingSystem os(1024, "idle.txt");
    os.loadProcess(1,"sharedmem1.txt", 4, 512, 3, 512);
    os.loadProcess(2,"sharedmem2.txt", 4, 512, 2, 512);
    os.start();
    return 0;
}

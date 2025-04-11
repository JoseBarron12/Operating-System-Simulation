#include "OperatingSystem.h"

int main(void) 
{
    OperatingSystem os(256, "prog1.txt");
    os.loadProcess(2, "prog2.txt", 256, 256, 2, 1024);
    os.start();
    return 0;
}

#include "OperatingSystem.h"

int main(void) 
{
    OperatingSystem os(512, "idle.txt");
    os.loadProcess(1,"program4.txt", 4, 512, 3, 512);
    os.start();
    return 0;
}

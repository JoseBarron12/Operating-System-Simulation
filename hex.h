// *************************************************************************** 
// 
//   Jose Barron 
//   Z2013735
//   CSCI 480 PE1
// 
//   I certify that this is my own work and where appropriate an extension 
//   of the starter code provided for the assignment. 
// ***************************************************************************
#ifndef HEX_H_INC
#define HEX_H_INC

#include <iostream>
#include <cstdint>
#include <string>
#include <sstream>

class hex
{
public:
    static std::string to_hex8(uint8_t i);
    static std::string to_hex32(uint32_t i);
    static std::string to_hex0x32(uint32_t i); 
    static std::string to_hex0x20(uint32_t i);
    static std::string to_hex0x12(uint32_t i);   
};

#endif
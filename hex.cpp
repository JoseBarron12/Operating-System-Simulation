//***************************************************************************
//
// Jose Barron
// Z02013735
// Fall 2024 CSCI 463 - PE1 Computer Architecture & Syst Org
// Final Version 1: This program is intended to implement the existing
// hex class, and the exisiting memoery class to simulate memory
// Due Date: 11/08/24
// I certify that this is my own work and where appropriate an extension
// of the starter code provided for the assignment.
//
//***************************************************************************
#include "hex.h"
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <iomanip>
#include <sstream>

/*
 * Converts the 8 bits (i) into 2 hex digits with zeroes if needed
 */
std::string hex::to_hex8(uint8_t i)
{
    std::ostringstream os;
    os << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(i);
    return os.str();
}

/*
 * Converts the 32 bits (i) into 8 hex digits 
 */
std::string hex::to_hex32(uint32_t i)
{
    std::ostringstream os;
    os << std::hex << std::setw(8) << std::setfill('0') << i;
    return os.str();
}

/*
 * Converts a the 8 bits (i) into 8 hex digits with the 0xh format
 */
std::string hex::to_hex0x32(uint32_t i)
{
    return std::string("0x")+to_hex32(i);
}

/*
 * Returns the 20 least significant bits of i using a bitmask of 0xFFFFF
 * and & to zero out the rest of the other bits
 */

std::string hex::to_hex0x20(uint32_t i)
{
    uint32_t LSB_20 = i & 0xFFFFF;
    std::ostringstream os;
    os << std::hex  << std::setfill('0') << std::setw(5) << LSB_20;
    return "0x" + os.str();
}

/*
 * Returns the 12 least significant bits of i using a bitmask of 0xFFF
 * and & to zero out the rest of the other bits
 */
std::string hex::to_hex0x12(uint32_t i)
{
    uint32_t LSB_12 = i & 0xFFF;
    std::ostringstream os;
    os << std::hex << std::setfill('0') << std::setw(3) << LSB_12;
    return "0x" + os.str();
}
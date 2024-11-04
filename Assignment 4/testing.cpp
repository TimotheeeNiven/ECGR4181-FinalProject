// main.cpp
#include "ram.cpp"
#include <iostream>

int main() {
    // Instantiate the RAM object
    RAM ram;

    // Simulation tick counter
    int tickCounter = 0;

    // Test writing a value to RAM
    uint32_t testAddress = 0x004;  // Some arbitrary address within bounds
    uint32_t testValue = 0xDEADBEEF;
    std::cout << "Writing 0x" << std::hex << testValue << " to address 0x" << testAddress << std::dec << std::endl;
    ram.write(testAddress, testValue, tickCounter);

    // Print tick count after write
    std::cout << "Ticks after write: " << tickCounter << std::endl;

    // Test reading the value back from RAM
    uint32_t readValue = ram.read(testAddress, tickCounter);
    std::cout << "Read value 0x" << std::hex << readValue << " from address 0x" << testAddress << std::dec << std::endl;

    // Print tick count after read
    std::cout << "Ticks after read: " << tickCounter << std::endl;

    // Print a memory range for debugging (adjust start and end as needed)
    std::cout << "\nMemory dump from 0x000 to 0x010:" << std::endl;
    ram.print(0x000, 0x010);

    // Test the initialized random floating-point arrays
    std::cout << "\nMemory dump of ARRAY_A (0x400 to 0x420):" << std::endl;
    ram.print(0x400, 0x420);

    std::cout << "\nMemory dump of ARRAY_B (0x800 to 0x820):" << std::endl;
    ram.print(0x800, 0x820);

    return 0;
}

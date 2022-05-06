#include "external_sort.h"
#include <iostream>

const uint32_t DEFAULT_BLOCK_SIZE = 4096;  // standart 4KB block size for SSD
const uint32_t DEFAULT_MEMORY_SIZE = 134217728;  // memory span we have

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Please enter number of numbers to generate\n";
    return 1;
  }
  std::string filename = std::string(argv[1]);
  std::cout << filename << std::endl;
  sort(filename, "out.dat", DEFAULT_BLOCK_SIZE, DEFAULT_MEMORY_SIZE);
  blockedPrint("out.dat");
  return 0;
}

#include "external_sort.h"
#include <iostream>

// const uint32_t DEFAULT_MEMORY_SIZE = 134217728;  // memory span we have

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Please input available memory and input filename\n";
    return 1;
  }
  uint64_t availableMemory = atol(argv[1]);
  std::string filename = std::string(argv[2]);
  std::cout << filename << std::endl;
  external::sort(filename, "out.dat", availableMemory);
  external::printBinartyFile("out.dat");
  return 0;
}

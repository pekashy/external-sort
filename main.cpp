#include "external_sort.h"
#include <iostream>

const uint32_t DEFAULT_MEMORY_SIZE = 134217728;  // memory span we have

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Please enter number of numbers to generate\n";
    return 1;
  }
  std::string filename = std::string(argv[1]);
  std::cout << filename << std::endl;
  external::sort(filename, "out.dat", DEFAULT_MEMORY_SIZE);
  external::printBinartyFile("out.dat");
  return 0;
}

#include "external_sort.h"

const uint32_t DEFAULT_MEMORY_SIZE = 134217728;  // memory span we have

int main(int argc, char** argv) {
  std::string filename;
  uint64_t availableMemory;
  if (argc < 2) {
    filename = "input";
    availableMemory = DEFAULT_MEMORY_SIZE;
  } else {
    availableMemory = atol(argv[1]);
    filename = std::string(argv[2]);
  }
  external::sort(filename, "output", availableMemory);
  return 0;
}

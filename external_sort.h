#pragma once
#include <string>

namespace external {
void sort(const std::string& inputFilename,
          const std::string& outputFilename,
          uint32_t availableMemorySize);
void printBinartyFile(const std::string& filename);
}

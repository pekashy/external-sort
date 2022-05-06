#pragma once
#include <string>

void sort(const std::string& inputFilename,
          const std::string& outputFilename,
          uint32_t diskReadBlockSize,
          uint32_t availableMemorySiz);
void blockedPrint(const std::string& filename);

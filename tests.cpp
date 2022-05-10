#include "external_sort.h"

#include <gtest/gtest.h>
#include <fstream>
#include <algorithm>

using namespace external;

void assertSortedCorrectly(const std::string& fliename) {
  std::ifstream file(fliename, std::ios::in | std::ios::binary);
  uint64_t filesize;
  file.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
  std::vector<uint32_t> vec;
  vec.resize(filesize);
  std::vector<uint32_t> rvector;
  rvector.resize(filesize);
  file.read(reinterpret_cast<char*>(&rvector[0]), filesize * sizeof(rvector[0]));
  auto sorted = rvector;
  std::sort(std::begin(sorted), std::end(sorted));
  ASSERT_EQ(rvector, sorted);
}

TEST(StandartCases, LessThanReadBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(StandartCases, BlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/128elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(StandartCases, DoubleBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/256elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(StandartCases, MoreThanBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/1000elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(MemoryCases, MemoryEqualsToReadBlockSizeSmallArr) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(MemoryCases, MemoryEqualsToReadBlockSizeEqualsToArrSize) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/128elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(MemoryCases, MemoryEqualsToReadBlockSizeEqualsToHalfOfArrSize) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/256elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(MemoryCases, MemoryEqualsToReadBlockSizeBiggerArr) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10000elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

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

TEST(IOBlockCases, LessThanReadBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(IOBlockCases, BlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/128elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(IOBlockCases, DoubleBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/256elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(IOBlockCases, MoreThanBlockSizeTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/1000elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, SmallArr) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, MemoryEqualsToReadBlockSizeEqualsToArrSize) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/128elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, Sort256) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/256elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, Sort10K) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10000elements.dat", outputFileName, 12288);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, Sort1M) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/1000000elements.dat", outputFileName, 134217728);
  assertSortedCorrectly(outputFileName);
}

TEST(SortingCases, Sort1M3MbMemory) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/1000000elements.dat", outputFileName, 3145728);
  assertSortedCorrectly(outputFileName);
}

TEST(TechCases, Sort1MVerificationTime) {
  std::ifstream file("../test_data/1000000elements.dat", std::ios::in | std::ios::binary);
  uint64_t filesize;
  file.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
  std::vector<uint32_t> vec;
  vec.resize(filesize);
  std::vector<uint32_t> rvector;
  rvector.resize(filesize);
  file.read(reinterpret_cast<char*>(&rvector[0]), filesize * sizeof(rvector[0]));
  auto sorted = rvector;
  std::sort(std::begin(sorted), std::end(sorted));
}

TEST(EdgeCases, NotEnoughMemory) {
  std::string outputFileName = std::tmpnam(nullptr);
  EXPECT_ANY_THROW(sort("../test_data/256elements.dat", outputFileName, 4096));
}

TEST(EdgeCases, NotExistingFile) {
  std::string outputFileName = std::tmpnam(nullptr);
  EXPECT_ANY_THROW(sort("filenotexists", outputFileName, 134217728));
}

TEST(EdgeCases, EmptyOutputFileName) {
  EXPECT_ANY_THROW(sort("../test_data/256elements.dat", "", 134217728));
}

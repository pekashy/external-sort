#include "external_sort.h"

#include <gtest/gtest.h>
#include <fstream>
#include <algorithm>

void assertSortedCorrectly(const std::string& fliename) {
  std::ifstream file(fliename, std::ios::in | std::ios::binary);
  uint64_t filesize;
  file.read(reinterpret_cast<char*>(&filesize), sizeof(filesize));
  std::vector<uint32_t> vec;
  vec.resize(filesize);
  std::vector<uint32_t> rvector;
  rvector.resize(filesize);
  file.read(reinterpret_cast<char*>(&rvector[0]), filesize*sizeof(rvector[0]));
  auto sorted = rvector;
  std::sort(std::begin(sorted), std::end(sorted));
  ASSERT_EQ(rvector, sorted);
}

TEST(BasicSort, HelloWorldTest) {
  std::string outputFileName = std::tmpnam(nullptr);
  sort("../test_data/10elements.dat", outputFileName, 4096,134217728);
  assertSortedCorrectly(outputFileName);
}
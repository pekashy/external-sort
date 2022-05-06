#include "external_sort.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <utility>
#include <queue>
#include <functional>
#include <set>
#include <memory>
#include <list>

namespace {
uint32_t BLOCK_SIZE = 4096;  // standart 4KB block numberOfElements for SSD
uint32_t MEMORY_SIZE = 134217728;  // memory span we have

struct SortedPartitionFIle {
  SortedPartitionFIle(std::string name, uint64_t sz)
      : filename(std::move(name)), numberOfElements(sz) {
  }
  std::string filename;
  uint64_t numberOfElements;
};

struct OpenedSortedPartitionFile {
  std::ifstream filestream;
  std::uint64_t elementsLeft;
};

uint32_t getSubarraySize() { // number of items we can sort inplace
  return MEMORY_SIZE / sizeof(uint32_t);
}

uint32_t getNumberOfBlocksToMerge() { // as each open file keeps an open buffer
  return MEMORY_SIZE / BLOCK_SIZE;
}

uint64_t getNumberOfItemsToSort(std::ifstream& infile) {
  uint64_t size;
  infile.read(reinterpret_cast<char*>(&size), sizeof(size));
  return size;
}

uint64_t getNumberOfItemsToSort(const std::string& infilename) {
  std::ifstream infile(infilename, std::ios::in | std::ios::binary);
  if (!infile.is_open()) {
    throw std::runtime_error("Please specify a correct file");
  }
  return getNumberOfItemsToSort(infile);
}

uint64_t getNumberOfBlocksForTheFirstMerge(uint64_t numberOfItemsTotal) {
  return (numberOfItemsTotal - 1) % (getNumberOfBlocksToMerge() - 1);
}

void readSortWrite(std::ifstream& input, std::ofstream& output, uint64_t number) {
  std::vector<uint32_t> inMemory;
  inMemory.resize(number);
  input.read(reinterpret_cast<char*>(&inMemory[0]), sizeof(inMemory[0]) * number);
  std::sort(std::begin(inMemory), std::end(inMemory));
  output.write(reinterpret_cast<char*>(&inMemory[0]), sizeof(inMemory[0]) * number);
}

std::ofstream openFileWriteElementsQuantity(const std::string& filename, uint64_t elementsQuantity) {
  std::ofstream out(filename, std::ios::out | std::ios::binary);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  out.write(reinterpret_cast<char*>(&elementsQuantity), sizeof(elementsQuantity));
  return out;
}

std::vector<SortedPartitionFIle> sortAndSplit(const std::string& filename) {
  std::ifstream infile(filename, std::ios::in | std::ios::binary);
  if (!infile.is_open()) {
    throw std::runtime_error("Please specify a correct file");
  }

  std::vector<SortedPartitionFIle> sortedPartitionFiles;
  uint64_t numberOfItems = getNumberOfItemsToSort(infile);
  for (uint64_t offset = 0; offset < numberOfItems; offset += getSubarraySize()) {
    uint64_t
        iterationSize =
        std::min(static_cast<uint64_t>(getSubarraySize()), static_cast<uint64_t>(numberOfItems - offset));
    std::string partitionFileName = std::tmpnam(nullptr);
    auto partitionFile = openFileWriteElementsQuantity(partitionFileName, iterationSize);
    readSortWrite(infile, partitionFile, iterationSize);
    sortedPartitionFiles.emplace_back(partitionFileName, iterationSize);
  }

  return sortedPartitionFiles;
}

OpenedSortedPartitionFile openSortedPartitonFile(const std::string& filename, size_t diskBlockSize) {
  std::ifstream infile(filename, std::ios::in | std::ios::binary);
  if (!infile.is_open()) {
    throw std::runtime_error("Please specify a correct file");
  }
  infile.rdbuf()->pubsetbuf(0, 0);
  uint64_t numberOfItems = getNumberOfItemsToSort(infile);
  return {std::move(infile), numberOfItems};
}

std::vector<uint32_t> readNextBatch(OpenedSortedPartitionFile& file) {
  size_t elementsToRead =
      std::min(static_cast<uint32_t>(file.elementsLeft), static_cast<uint32_t>(BLOCK_SIZE / sizeof(uint32_t)));
  std::vector<uint32_t> batch;
  batch.resize(elementsToRead);
  file.filestream.read(reinterpret_cast<char*>(&batch[0]), elementsToRead * sizeof(uint32_t));
  file.elementsLeft -= elementsToRead;
  return batch;
}

SortedPartitionFIle mergePartitionFiles(const std::vector<SortedPartitionFIle>& filesToSort, size_t diskBlockSize) {
  std::string outputPartitionFileName = std::tmpnam(nullptr);
  std::multiset<uint32_t> mergeset;
  std::list<OpenedSortedPartitionFile> openedFilesList;
  uint64_t elementsToMerge = 0;
  for (auto& fileDescription: filesToSort) {
    openedFilesList.push_back(openSortedPartitonFile(fileDescription.filename, diskBlockSize));
    elementsToMerge += fileDescription.numberOfElements;
  }
  auto outputPartitionFile = openFileWriteElementsQuantity(outputPartitionFileName, elementsToMerge);
  while (!openedFilesList.empty()) {
    std::vector<uint32_t> merged;
    for (auto openedFileIt = std::begin(openedFilesList); openedFileIt != std::end(openedFilesList);) {
      if (openedFileIt->elementsLeft == 0) {
        openedFileIt = openedFilesList.erase(openedFileIt);
        continue;
      }
      auto batch = readNextBatch(*openedFileIt);
      std::copy(std::make_move_iterator(std::begin(batch)),
                std::make_move_iterator(std::end(batch)),
                std::inserter(merged, std::begin(merged)));
      openedFileIt++;
    }
    std::sort(std::begin(merged), std::end(merged));
    outputPartitionFile.write(reinterpret_cast<char*>(&merged[0]), sizeof(merged[0]) * merged.size());
  }

  return {outputPartitionFileName, elementsToMerge};
}

using smallestBlocksHeapType = std::priority_queue<SortedPartitionFIle,
                                                   std::vector<SortedPartitionFIle>,
                                                   std::function<bool(const SortedPartitionFIle& lhs,
                                                                      const SortedPartitionFIle& rhs)>>;
std::vector<SortedPartitionFIle> pullNSmallestPartitionsFromHeap(smallestBlocksHeapType& blockHeap, uint64_t N) {
  std::vector<SortedPartitionFIle> nSmallest;
  for (int i = 0; i < N; ++i) {
    if (blockHeap.empty()) {
      throw std::runtime_error("Heap numberOfElements miscalculation!");
    }

    nSmallest.push_back(blockHeap.top());
    blockHeap.pop();
  }

  return nSmallest;
}

std::string mergeSort(const std::string& inputFilename) {
  auto partitionNames = sortAndSplit(inputFilename);
  auto getSmallestPartitionSortF = [](const SortedPartitionFIle& lhs, const SortedPartitionFIle& rhs) {
    return lhs.numberOfElements > rhs.numberOfElements;
  };
  auto sortedPartitions = sortAndSplit(inputFilename);
  if (sortedPartitions.empty()) {
    throw std::runtime_error("No partiton files were created!");
  }

  if (sortedPartitions.size() == 1) {
    return sortedPartitions[0].filename;
  }

  smallestBlocksHeapType smallestBlocksHeap(std::begin(sortedPartitions),
                                            std::end(sortedPartitions), getSmallestPartitionSortF);
  auto totalItemsNumber = getNumberOfItemsToSort(inputFilename);

  auto firstMergeSize = getNumberOfBlocksForTheFirstMerge(totalItemsNumber);
  if (firstMergeSize > 0) {
    auto firstMergeBatch = pullNSmallestPartitionsFromHeap(smallestBlocksHeap, firstMergeSize);
    auto mergedPartiton = mergePartitionFiles(firstMergeBatch, BLOCK_SIZE);
    smallestBlocksHeap.push(mergedPartiton);
  }

  while (smallestBlocksHeap.size() > 1) {
    auto mergeBatch = pullNSmallestPartitionsFromHeap(smallestBlocksHeap, getNumberOfBlocksToMerge());
    auto mergedPartiton = mergePartitionFiles(mergeBatch, BLOCK_SIZE);
    smallestBlocksHeap.push(mergedPartiton);
  }

  return smallestBlocksHeap.top().filename;
}
}

void sort(const std::string& inputFilename,
          const std::string& outputFilename,
          uint32_t diskReadBlockSize,
          uint32_t availableMemorySize) {
  BLOCK_SIZE = diskReadBlockSize;
  MEMORY_SIZE = availableMemorySize;

  auto outPartitionName = mergeSort(inputFilename);
  std::ifstream src(outPartitionName, std::ios::binary);
  std::ofstream dst(outputFilename, std::ios::binary);
  dst << src.rdbuf();
}

void blockedPrint(const std::string& filename) {
  auto openedFile = openSortedPartitonFile(filename, BLOCK_SIZE);
  while (openedFile.elementsLeft > 0) {
    auto batch = readNextBatch(openedFile);
    for (uint32_t elem: batch) {
      std::cout << elem << " ";
    }
  }
}

#include "external_sort.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <utility>
#include <queue>
#include <functional>
#include <list>

namespace {
uint32_t BLOCK_SIZE = 4096;  // standart 4KB block for SSD
uint32_t MEMORY_SIZE = 134217728;  // memory span we have

struct SortedPartitionFIleMetadata {
  SortedPartitionFIleMetadata(std::string name, uint64_t sz)
      : filename(std::move(name)), numberOfElements(sz) {
  }
  std::string filename;
  uint64_t numberOfElements;
};

struct OpenedSortedPartitionFile {
  OpenedSortedPartitionFile(const std::string& filename);
  std::shared_ptr<std::ifstream> filestream; // as we need to put this object into priority_queue
  uint64_t elementsLeft;
  uint32_t frontElement;
 private:
  std::shared_ptr<char[]> fileBuffer;
};

struct OutputFile {
  OutputFile(const std::string& filename);
  std::ofstream outStream;
 private:
  std::unique_ptr<char[]> fileBuffer;
};

uint32_t getSubarraySize() { // number of items we can sort inplace
  return MEMORY_SIZE / sizeof(uint32_t);
}

uint32_t getMaxNumberOfBlocksInMemory() { // as each open file keeps an open buffer
  return MEMORY_SIZE / BLOCK_SIZE - 1; // as we usually need to keep a buffer to output stream open
}

uint64_t getNumberOfBlocksForTheFirstMerge(uint64_t numberOfItemsTotal) {
  return (numberOfItemsTotal - 1) % (getMaxNumberOfBlocksInMemory() - 1);
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

OpenedSortedPartitionFile::OpenedSortedPartitionFile(const std::string& filename) {
  filestream = std::make_shared<std::ifstream>(filename, std::ios::in | std::ios::binary);
  if (!filestream->is_open()) {
    throw std::runtime_error("Please specify a correct file");
  }
  fileBuffer = std::make_unique<char[]>(BLOCK_SIZE); // std::make_shared<char[]> is Cpp17+
  filestream->rdbuf()->pubsetbuf(fileBuffer.get(), BLOCK_SIZE);
}

OutputFile::OutputFile(const std::string& filename) {
  outStream = std::ofstream(filename, std::ios::out | std::ios::binary);
  if (!outStream.is_open()) {
    throw std::runtime_error("Failed to open file %s" + filename);
  }
  fileBuffer = std::make_unique<char[]>(BLOCK_SIZE);
  outStream.rdbuf()->pubsetbuf(fileBuffer.get(), BLOCK_SIZE);
}

void readSortWrite(std::ifstream& input, std::ofstream& output, uint64_t number) {
  std::vector<uint32_t> inMemory;
  inMemory.resize(number);
  input.read(reinterpret_cast<char*>(&inMemory[0]), sizeof(inMemory[0]) * number);
  std::sort(std::begin(inMemory), std::end(inMemory));
  output.write(reinterpret_cast<char*>(&inMemory[0]), sizeof(inMemory[0]) * number);
}

OutputFile openFileWriteElementsQuantity(const std::string& filename, uint64_t elementsQuantity) {
  OutputFile outfile(filename);
  outfile.outStream.write(reinterpret_cast<char*>(&elementsQuantity), sizeof(elementsQuantity));
  return outfile;
}

std::vector<SortedPartitionFIleMetadata> sortAndSplit(const std::string& filename) {
  std::ifstream infile(filename, std::ios::in | std::ios::binary);
  if (!infile.is_open()) {
    throw std::runtime_error("Please specify a correct file");
  }

  std::vector<SortedPartitionFIleMetadata> sortedPartitionFiles;
  uint64_t numberOfItems = getNumberOfItemsToSort(infile);
  for (uint64_t offset = 0; offset < numberOfItems;
       offset += getSubarraySize()) {
    uint64_t iterationSize =
        std::min(static_cast<uint64_t>(getSubarraySize()), static_cast<uint64_t>(numberOfItems - offset));
    std::string partitionFileName = std::tmpnam(nullptr);
    auto partitionFile = openFileWriteElementsQuantity(partitionFileName, iterationSize);
    readSortWrite(infile, partitionFile.outStream, iterationSize);
    sortedPartitionFiles.emplace_back(partitionFileName, iterationSize);
  }

  return sortedPartitionFiles;
}

bool readNextNum(OpenedSortedPartitionFile& file) {
  if (file.elementsLeft <= 0) {
    return false;
  }
  uint32_t newFront;
  file.filestream->read(reinterpret_cast<char*>(&newFront), sizeof(uint32_t));
  file.elementsLeft -= 1;
  file.frontElement = newFront;
  return true;
}

OpenedSortedPartitionFile openSortedPartitonFile(const std::string& filename) {
  OpenedSortedPartitionFile openedFile(filename);
  uint64_t numberOfItems = getNumberOfItemsToSort(*openedFile.filestream);
  openedFile.elementsLeft = numberOfItems;
  readNextNum(openedFile);
  return openedFile;
}

SortedPartitionFIleMetadata mergePartitionFiles(const std::vector<SortedPartitionFIleMetadata>& filesToSort) {
  std::string outputPartitionFileName = std::tmpnam(nullptr);
  auto sortOpenFilesFunction = [](const OpenedSortedPartitionFile& lhs, const OpenedSortedPartitionFile& rhs) {
    return lhs.frontElement > rhs.frontElement;
  };

  using mergeQueueType = std::priority_queue<OpenedSortedPartitionFile,
                                             std::vector<OpenedSortedPartitionFile>,
                                             decltype(sortOpenFilesFunction)>;
  mergeQueueType mergeQueue(sortOpenFilesFunction);
  uint64_t elementsToMerge = 0;
  for (auto& fileDescription: filesToSort) {
    mergeQueue.push(openSortedPartitonFile(fileDescription.filename));
    elementsToMerge += fileDescription.numberOfElements;
  }
  auto outputPartitionFile = openFileWriteElementsQuantity(outputPartitionFileName, elementsToMerge);
  while (!mergeQueue.empty()) {
    auto fileWithSmallestElem = mergeQueue.top();
    mergeQueue.pop();
    outputPartitionFile.outStream.write(reinterpret_cast<char*>(&fileWithSmallestElem.frontElement),
                                        sizeof(fileWithSmallestElem.frontElement));
    if (readNextNum(fileWithSmallestElem)) {
      mergeQueue.push(fileWithSmallestElem);
    }
  }

  return {outputPartitionFileName, elementsToMerge};
}

using smallestBlocksHeapType = std::priority_queue<SortedPartitionFIleMetadata,
                                                   std::vector<SortedPartitionFIleMetadata>,
                                                   std::function<bool(const SortedPartitionFIleMetadata& lhs,
                                                                      const SortedPartitionFIleMetadata& rhs)>>;
std::vector<SortedPartitionFIleMetadata> pullNSmallestPartitionsFromHeap(smallestBlocksHeapType& blockHeap,
                                                                         uint64_t N) {
  std::vector<SortedPartitionFIleMetadata> nSmallest;
  for (int i = 0; i < N && !blockHeap.empty(); ++i) {
    nSmallest.push_back(blockHeap.top());
    blockHeap.pop();
  }

  return nSmallest;
}

std::string mergeSort(const std::string& inputFilename) {
  auto partitionNames = sortAndSplit(inputFilename);
  auto getSmallestPartitionSortF = [](const SortedPartitionFIleMetadata& lhs, const SortedPartitionFIleMetadata& rhs) {
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
    auto mergedPartiton = mergePartitionFiles(firstMergeBatch);
    smallestBlocksHeap.push(mergedPartiton);
  }

  while (smallestBlocksHeap.size() > 1) {
    auto mergeBatch = pullNSmallestPartitionsFromHeap(smallestBlocksHeap, getMaxNumberOfBlocksInMemory());
    auto mergedPartiton = mergePartitionFiles(mergeBatch);
    smallestBlocksHeap.push(mergedPartiton);
  }

  return smallestBlocksHeap.top().filename;
}
}

void external::sort(const std::string& inputFilename,
                    const std::string& outputFilename,
                    uint32_t availableMemorySize) {
  MEMORY_SIZE = availableMemorySize;
  if (getMaxNumberOfBlocksInMemory() < 2) {
    throw std::runtime_error("Please provide more memory, as we are unable to fit two blocks in memory to merge them.");
  }
  auto outPartitionName = mergeSort(inputFilename);
  std::ifstream src(outPartitionName, std::ios::binary);
  std::ofstream dst(outputFilename, std::ios::binary);
  dst << src.rdbuf();
}

std::vector<uint32_t> readNextBatch(OpenedSortedPartitionFile& file) {
  size_t elementsToRead =
      std::min(static_cast<uint32_t>(file.elementsLeft), static_cast<uint32_t>(BLOCK_SIZE / sizeof(uint32_t)));
  std::vector<uint32_t> batch;
  batch.resize(elementsToRead);
  file.filestream->read(reinterpret_cast<char*>(&batch[0]), elementsToRead * sizeof(uint32_t));
  file.elementsLeft -= elementsToRead;
  return batch;
}

void external::printBinartyFile(const std::string& filename) {
  auto openedFile = openSortedPartitonFile(filename);
  while (openedFile.elementsLeft > 0) {
    auto batch = readNextBatch(openedFile);
    for (uint32_t elem: batch) {
      std::cout << elem << " ";
    }
  }
}

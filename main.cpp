#include <iostream>
#include <random>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <utility>
#include <queue>
#include <functional>
#include <set>
#include <memory>
#include <list>

const uint32_t BLOCK_SIZE = 4096;  // standart 4KB block size for SSD
const uint32_t MEMORY_SIZE = 134217728;  // memory span we have
constexpr uint32_t SUBARRAY_SIZE = MEMORY_SIZE / sizeof(uint32_t); // number of items we can sort inplace
constexpr uint32_t NUMBER_OF_BLOCKS_TO_MERGE =
		MEMORY_SIZE / BLOCK_SIZE; // as each open file keeps an open buffer

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
	return (numberOfItemsTotal - 1) % (NUMBER_OF_BLOCKS_TO_MERGE - 1);
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

struct SortedPartitionFIle {
	SortedPartitionFIle(std::string name, uint64_t sz)
			: filename(std::move(name)), size(sz) {
	}
	std::string filename;
	uint64_t size;
};

std::vector<SortedPartitionFIle> sortAndSplit(const std::string& filename) {
	std::ifstream infile(filename, std::ios::in | std::ios::binary);
	if (!infile.is_open()) {
		throw std::runtime_error("Please specify a correct file");
	}

	std::vector<SortedPartitionFIle> sortedPartitionFiles;
	uint64_t numberOfItems = getNumberOfItemsToSort(infile);
	for (uint64_t offset = 0; offset < numberOfItems; offset += SUBARRAY_SIZE) {
		uint64_t
				iterationSize = std::min(static_cast<uint64_t>(SUBARRAY_SIZE), static_cast<uint64_t>(numberOfItems - offset));
		std::string partitionFileName = std::tmpnam(nullptr);
		auto partitionFile = openFileWriteElementsQuantity(partitionFileName, iterationSize);
		readSortWrite(infile, partitionFile, iterationSize);
		sortedPartitionFiles.emplace_back(partitionFileName, iterationSize);
	}

	return sortedPartitionFiles;
}

struct OpenedSortedPartitionFile {
	std::ifstream filestream;
	std::uint64_t elementsLeft;
};

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
		elementsToMerge += fileDescription.size;
	}
	auto outputPartitionFile = openFileWriteElementsQuantity(outputPartitionFileName, elementsToMerge);
	while (!openedFilesList.empty()) {
		std::vector<uint32_t> merged;
		// sorted.reserve(elementsToMerge); - will fast things up a bit, but we are already out of memory
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
		outputPartitionFile.write(reinterpret_cast<char*>(&merged[0]), sizeof(merged[0])*merged.size());
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
			throw std::runtime_error("Heap size miscalculation!");
		}

		nSmallest.push_back(blockHeap.top());
		blockHeap.pop();
	}

	return nSmallest;
}

std::string mergeSort(const std::string& infilename) {
	auto partitionNames = sortAndSplit(infilename);
	auto getSmallestPartitionSortF = [](const SortedPartitionFIle& lhs, const SortedPartitionFIle& rhs) {
		return lhs.size > rhs.size;
	};
	auto sortedPartitions = sortAndSplit(infilename);
	if (sortedPartitions.empty()) {
		throw std::runtime_error("No partiton files were created!");
	}

	if (sortedPartitions.size() == 1) {
		return sortedPartitions[0].filename;
	}

	smallestBlocksHeapType smallestBlocksHeap(std::begin(sortedPartitions),
																						std::end(sortedPartitions), getSmallestPartitionSortF);
	auto totalItemsNumber = getNumberOfItemsToSort(infilename);

	auto firstMergeSize = getNumberOfBlocksForTheFirstMerge(totalItemsNumber);
	auto firstMergeBatch = pullNSmallestPartitionsFromHeap(smallestBlocksHeap, firstMergeSize);
	auto mergedPartiton = mergePartitionFiles(firstMergeBatch, BLOCK_SIZE);
	smallestBlocksHeap.push(mergedPartiton);

	while (smallestBlocksHeap.size() > 1) {
		auto mergeBatch = pullNSmallestPartitionsFromHeap(smallestBlocksHeap, NUMBER_OF_BLOCKS_TO_MERGE);
		mergedPartiton = mergePartitionFiles(firstMergeBatch, BLOCK_SIZE);
		smallestBlocksHeap.push(mergedPartiton);
	}

	return smallestBlocksHeap.top().filename;
}

void print(const std::string& filename) {
	auto openedFile = openSortedPartitonFile(filename, BLOCK_SIZE);
	while(openedFile.elementsLeft > 0) {
		auto batch = readNextBatch(openedFile);
		for(uint32_t elem : batch) {
			std::cout << elem << " ";
		}
		std::cout << std::endl;
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Please enter number of numbers to generate\n";
		return 1;
	}
	std::string filename = std::string(argv[1]);
	std::cout << filename << std::endl;
	auto outFilename = mergeSort(filename);
	std::ifstream  src(outFilename, std::ios::binary);
	std::ofstream  dst("out.dat",   std::ios::binary);
	dst << src.rdbuf();
	dst.close();
	print("out.dat");
	return 0;
}

#include <iostream>
#include <random>
#include <fstream>

int main(int argc, char** argv) {
	if(argc < 2) {
		std::cerr << "Please enter number of numbers to generate\n";
		return 1;
	}
	std::random_device device;
	std::mt19937 rng(device());
	std::uniform_int_distribution<std::mt19937::result_type> dist;
	uint64_t quantity = atol(argv[1]);
	if(quantity < 1) {
		return 0;
	}

	std::vector<uint32_t> numbers;
	numbers.reserve(quantity);
	for(int i = 0; i < quantity; ++i) {
		uint32_t number = dist(rng);
		numbers.push_back(number);
		std::cout << number << " ";
	}
	std::cout << std::endl;
	std::ofstream out("input.dat", std::ios::out | std::ios::binary);
	out.write(reinterpret_cast<char*>(&quantity), sizeof(quantity));
	out.write(reinterpret_cast<char*>(&numbers[0]), quantity*sizeof(uint32_t));
	return 0;
}

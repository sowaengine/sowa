#include "Random.hpp"

#include <chrono>
#include <random>

namespace Ease::Random {

size_t GenerateID() {
	size_t id = 0;

	RandomNumberGenerator gen{};
	gen.Generate(id);
	return id;
}
uint32_t GenerateID32() {
	uint32_t id = 0;

	RandomNumberGenerator gen{};
	gen.Generate(id);
	return id;
}

//
//
//
RandomNumberGenerator::RandomNumberGenerator() {
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	_Engine.seed(time);
}

RandomNumberGenerator::RandomNumberGenerator(size_t seed) {
	_Engine.seed(seed);
}

void RandomNumberGenerator::Generate(size_t &number, size_t begin /*= 0*/, size_t end /*= SIZE_MAX*/) {
	std::uniform_int_distribution<size_t> dist(begin, end);
	number = dist(_Engine);
}

void RandomNumberGenerator::Generate(uint32_t &number, uint32_t begin /*= 0*/, uint32_t end /*= UINT_MAX*/) {
	std::uniform_int_distribution<uint32_t> dist(begin, end);
	number = dist(_Engine);
}

void RandomNumberGenerator::Generate(double &number, double begin /*= 0.0*/, double end /*= 1.0*/) {
	std::uniform_real_distribution<double> dist(begin, end);
	number = dist(_Engine);
}

} // namespace Ease::Random

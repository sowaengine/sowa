#include "Random.hpp"

#include "stlpch.hpp"

namespace Sowa {

uint64_t Random::GenerateID() {
	size_t id = 0;

	RandomNumberGenerator gen{};
	gen.Generate(id);
	return id;
}
int32_t Random::GenerateID31() {
	int32_t id = 0;

	RandomNumberGenerator gen{};
	gen.Generate(id);
	return id;
}
uint32_t Random::GenerateID32() {
	uint32_t id = 0;

	RandomNumberGenerator gen{};
	gen.Generate(id);
	return id;
}

//
//
//
Random::RandomNumberGenerator::RandomNumberGenerator() {
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	_Engine.seed(time);
}

Random::RandomNumberGenerator::RandomNumberGenerator(size_t seed) {
	_Engine.seed(seed);
}

void Random::RandomNumberGenerator::Generate(uint64_t &number, uint64_t begin /*= 0*/, uint64_t end /*= SIZE_MAX*/) {
	std::uniform_int_distribution<size_t> dist(begin, end);
	number = dist(_Engine);
}

void Random::RandomNumberGenerator::Generate(uint32_t &number, uint32_t begin /*= 0*/, uint32_t end /*= UINT_MAX*/) {
	std::uniform_int_distribution<uint32_t> dist(begin, end);
	number = dist(_Engine);
}

void Random::RandomNumberGenerator::Generate(int32_t &number, int32_t begin /*= 0*/, int32_t end /*= INT_MAX*/) {
	std::uniform_int_distribution<int32_t> dist(begin, end);
	number = dist(_Engine);
}

void Random::RandomNumberGenerator::Generate(double &number, double begin /*= 0.0*/, double end /*= 1.0*/) {
	std::uniform_real_distribution<double> dist(begin, end);
	number = dist(_Engine);
}
} // namespace Sowa

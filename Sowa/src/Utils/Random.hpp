#ifndef _E_RANDOM_HPP__
#define _E_RANDOM_HPP__
#pragma once

#include <limits.h>
#include <random>
#include <stdlib.h>

namespace Sowa {
namespace Random {

uint64_t GenerateID();
int32_t GenerateID31();
uint32_t GenerateID32();

class RandomNumberGenerator {
  public:
	RandomNumberGenerator();
	RandomNumberGenerator(size_t seed);

	// generate number between 0 and SIZE_MAX (2^64)
	void Generate(uint64_t &number, uint64_t begin = 0, uint64_t end = SIZE_MAX);

	// generate number between 0 and UINT_MAX (2^32)
	void Generate(uint32_t &number, uint32_t begin = 0, uint32_t end = UINT_MAX);

	// generate number between 0 and INT_MAX (2^31)
	void Generate(int32_t &number, int32_t begin = 0, int32_t end = INT_MAX);

	// generate number between 0.0 and 1.0
	void Generate(double &number, double begin = 0.0, double end = 1.0);

  private:
	std::default_random_engine _Engine{};
};
} // namespace Random
} // namespace Sowa

#endif // _E_DIALOG_HPP__
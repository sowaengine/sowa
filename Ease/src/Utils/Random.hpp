#ifndef _E_RANDOM_HPP__
#define _E_RANDOM_HPP__
#pragma once

#include <random>
#include <stdlib.h>

namespace Ease::Random {

size_t GenerateID();

class RandomNumberGenerator {
  public:
	RandomNumberGenerator();
	RandomNumberGenerator(size_t seed);

	// generate number between 0 and SIZE_MAX (2^64)
	void Generate(size_t &number, size_t begin = 0, size_t end = SIZE_MAX);

	// generate number between 0.0 and 1.0
	void Generate(double &number, double begin = 0.0, double end = 1.0);

  private:
	std::default_random_engine _Engine{};
};
} // namespace Ease::Random

#endif // _E_DIALOG_HPP__
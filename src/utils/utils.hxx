#ifndef SW_UTILS_HXX
#define SW_UTILS_HXX
#pragma once

#include <string>
#include <vector>

namespace Utils {

//-------------------------   String  -------------------------//

std::vector<std::string> Split(std::string str, std::string delimiter);

//-------------------------   Random  -------------------------//

/// @brief Randomizes random number generation
void Randomize();

/// @brief Wraps rand() function
int Rand();

/// @brief Returns an int between min and max.
/// max >= min, swaps min and max
/// max == min, returns max
int RandRange(int min, int max);

/// @brief Returns a random float between 0 and 1
float RandFloat();

/// @brief Returns a float between min and max.
/// max >= min, swaps min and max
/// max == min, returns min
float RandRangeFloat(float min, float max);

} // namespace Utils

#endif // SW_UTILS_HXX
#include "utils.hxx"

void Utils::Randomize() {
	srand(static_cast<unsigned>(time(NULL)));
}

int Utils::Rand() {
	return rand();
}

int Utils::RandRange(int min, int max) {
	if (min == max)
		return max;
	if (max < min)
		std::swap(min, max);

	return min + (Rand() % (max - min));
}

float Utils::RandFloat() {
	return static_cast<float>(Rand()) / static_cast<float>(RAND_MAX);
}

float Utils::RandRangeFloat(float min, float max) {
	if (max < min)
		std::swap(min, max);

	return min + (RandFloat() * (max - min));
}
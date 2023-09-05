#include "time.hxx"

double Time::delta;
std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> Time::lastUpdate;

double Time::Delta() {
	return Time::delta;
}

void Time::update() {
	std::chrono::time_point now = std::chrono::system_clock::now();

	std::chrono::duration<double, std::milli> ms = now - Time::lastUpdate;
	Time::delta = ms.count() * 0.001;

	Time::lastUpdate = now;
}
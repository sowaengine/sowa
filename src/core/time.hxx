#ifndef SW_TIME_HXX
#define SW_TIME_HXX
#pragma once

#include <chrono>

class App;

class Time {
  public:
	static double Delta();

  private:
	friend class App;

	static void update();

	static std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> lastUpdate;
	static double delta;
};

#endif // SW_TIME_HXX
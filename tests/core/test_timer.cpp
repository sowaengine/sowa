#include <doctest/doctest.h>

#include "core/timer.hpp"

using namespace sowa;

TEST_CASE("Timer Start/Pause/Stop state machine") {
	Timer t(1.f);
	t.Start();
	CHECK(t.IsRunning());

	CHECK(t.Update(0.4f) == false);
	CHECK(t.Elapsed() == doctest::Approx(0.4f));

	t.Pause();
	CHECK(t.IsPaused());
	CHECK(t.Elapsed() == doctest::Approx(0.4f));
	CHECK(t.Update(0.5f) == false); // paused: no advance
	CHECK(t.Elapsed() == doctest::Approx(0.4f));

	t.Start(); // resume
	CHECK(t.IsRunning());
	CHECK(t.Elapsed() == doctest::Approx(0.4f));

	t.Stop();
	CHECK(t.GetState() == Timer::State::Stopped);
	CHECK(t.Elapsed() == 0.f);
	CHECK(t.Update(1.f) == false);
}

TEST_CASE("Timer one-shot fires once then stops") {
	int count = 0;
	Timer t(1.f, true);
	t.SetOnTimeout([&]() { count++; });

	CHECK(t.Update(0.5f) == false);
	CHECK(count == 0);
	CHECK(t.Remaining() == doctest::Approx(0.5f));

	CHECK(t.Update(0.5f) == true);
	CHECK(count == 1);
	CHECK(t.GetState() == Timer::State::Stopped);
	CHECK(t.Elapsed() == 0.f);

	CHECK(t.Update(1.f) == false);
	CHECK(count == 1);
}

TEST_CASE("Timer repeating catches up on large delta") {
	int count = 0;
	Timer t(0.1f, true, true);
	t.SetOnTimeout([&]() { count++; });

	CHECK(t.Update(0.5f) == true);
	CHECK(count == 5);
	CHECK(t.IsRunning());
	CHECK(t.Elapsed() == doctest::Approx(0.f));

	CHECK(t.Update(0.25f) == true);
	CHECK(count == 7);
	CHECK(t.Elapsed() == doctest::Approx(0.05f));
}

TEST_CASE("Timer one-shot with large delta fires only once") {
	int count = 0;
	Timer t(0.1f, true);
	t.SetOnTimeout([&]() { count++; });

	CHECK(t.Update(0.5f) == true);
	CHECK(count == 1);
	CHECK(t.GetState() == Timer::State::Stopped);
}

TEST_CASE("Timer timeout <= 0 fires every Update while running") {
	int count = 0;
	Timer repeating(0.f, true, true);
	repeating.SetOnTimeout([&]() { count++; });

	CHECK(repeating.Remaining() == 0.f);
	CHECK(repeating.Update(0.016f) == true);
	CHECK(repeating.Update(1.f) == true);
	CHECK(count == 2);
	CHECK(repeating.IsRunning());

	int oneShotCount = 0;
	Timer oneShot(-1.f, true);
	oneShot.SetOnTimeout([&]() { oneShotCount++; });
	CHECK(oneShot.Update(0.f) == true);
	CHECK(oneShotCount == 1);
	CHECK(oneShot.GetState() == Timer::State::Stopped);
	CHECK(oneShot.Update(0.f) == false);
	CHECK(oneShotCount == 1);
}

TEST_CASE("Timer SetOnTimeout is stored for the next fire") {
	int count = 0;
	Timer t(1.f, true);
	CHECK(t.Update(1.f) == true); // fires with no callback
	CHECK(t.GetState() == Timer::State::Stopped);

	t.SetOnTimeout([&]() { count++; });
	t.Start();
	CHECK(t.Update(1.f) == true);
	CHECK(count == 1);

	t.ClearOnTimeout();
	t.Start();
	CHECK(t.Update(1.f) == true);
	CHECK(count == 1);
}

TEST_CASE("Timer SetTimeout / SetRepeating") {
	Timer t(2.f, true);
	CHECK(t.Timeout() == 2.f);
	t.SetTimeout(0.5f);
	CHECK(t.Timeout() == 0.5f);

	int count = 0;
	t.SetOnTimeout([&]() { count++; });
	t.SetRepeating(true);
	CHECK(t.IsRepeating());

	CHECK(t.Update(1.0f) == true);
	CHECK(count == 2);
	CHECK(t.IsRunning());
}

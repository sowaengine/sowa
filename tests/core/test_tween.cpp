#include <doctest/doctest.h>

#include "core/tween.hpp"

using namespace sowa;

TEST_CASE("Tween reports linear progress and finishes once") {
	f32 seen = -1.f;
	int finishes = 0;
	Tween t(1.f, true);
	t.SetOnUpdate([&](f32 value) { seen = value; });
	t.SetOnFinish([&]() { finishes++; });

	CHECK(t.Update(0.25f) == false);
	CHECK(seen == doctest::Approx(0.25f));
	CHECK(t.Progress() == doctest::Approx(0.25f));
	CHECK(t.Remaining() == doctest::Approx(0.75f));
	CHECK(finishes == 0);

	CHECK(t.Update(0.75f) == true);
	CHECK(seen == doctest::Approx(1.f));
	CHECK(t.IsFinished());
	CHECK(finishes == 1);

	CHECK(t.Update(1.f) == false);
	CHECK(finishes == 1);
}

TEST_CASE("Tween large delta jumps to the end") {
	int updates = 0;
	int finishes = 0;
	f32 seen = -1.f;
	Tween t(0.1f, true);
	t.SetOnUpdate([&](f32 value) {
		updates++;
		seen = value;
	});
	t.SetOnFinish([&]() { finishes++; });

	CHECK(t.Update(5.f) == true);
	CHECK(updates == 1);
	CHECK(seen == doctest::Approx(1.f));
	CHECK(finishes == 1);
	CHECK(t.IsFinished());
}

TEST_CASE("Tween duration <= 0 fires 1 and finishes") {
	f32 seen = -1.f;
	int finishes = 0;
	Tween t(0.f, true);
	t.SetOnUpdate([&](f32 value) { seen = value; });
	t.SetOnFinish([&]() { finishes++; });

	CHECK(t.Update(0.016f) == true);
	CHECK(seen == doctest::Approx(1.f));
	CHECK(finishes == 1);
	CHECK(t.IsFinished());
	CHECK(t.Update(0.016f) == false);
	CHECK(finishes == 1);
}

TEST_CASE("Tween pause holds elapsed and stop resets it") {
	f32 seen = -1.f;
	Tween t(1.f, true);
	t.SetOnUpdate([&](f32 value) { seen = value; });

	t.Update(0.4f);
	CHECK(seen == doctest::Approx(0.4f));
	t.Pause();
	CHECK(t.IsPaused());
	CHECK(t.Update(0.5f) == false);
	CHECK(t.Elapsed() == doctest::Approx(0.4f));

	t.Start();
	CHECK(t.IsRunning());
	t.Update(0.1f);
	CHECK(seen == doctest::Approx(0.5f));

	t.Stop();
	CHECK(t.GetState() == Tween::State::Stopped);
	CHECK(t.Elapsed() == 0.f);
	CHECK_FALSE(t.IsFinished());
}

TEST_CASE("Tween Start after finish restarts") {
	int finishes = 0;
	Tween t(0.2f, true);
	t.SetOnFinish([&]() { finishes++; });
	CHECK(t.Update(0.2f) == true);
	CHECK(finishes == 1);
	t.Start();
	CHECK(t.IsRunning());
	CHECK(t.Elapsed() == 0.f);
	CHECK(t.Update(0.2f) == true);
	CHECK(finishes == 2);
}

TEST_CASE("Tween Start from on_finish restarts") {
	int finishes = 0;
	Tween t(0.1f, true);
	t.SetOnFinish([&]() {
		finishes++;
		if (finishes == 1)
			t.Start();
	});
	CHECK(t.Update(0.1f) == true);
	CHECK(t.IsRunning());
	CHECK(finishes == 1);
	CHECK(t.Update(0.1f) == true);
	CHECK(finishes == 2);
	CHECK(t.IsFinished());
}

TEST_CASE("Tween SetEase maps the 0..1 progress") {
	f32 seen = -1.f;
	Tween t(1.f, true);
	t.SetEase([](f32 x) { return x * x; });
	t.SetOnUpdate([&](f32 value) { seen = value; });
	t.Update(0.5f);
	CHECK(seen == doctest::Approx(0.25f));
	t.ClearEase();
	t.Stop();
	t.Start();
	t.Update(0.5f);
	CHECK(seen == doctest::Approx(0.5f));
}

TEST_CASE("Tween SetEase Easing uses Math::Ease") {
	f32 seen = -1.f;
	Tween t(1.f, true);
	t.SetEase(Easing::QuadIn);
	t.SetOnUpdate([&](f32 value) { seen = value; });
	t.Update(0.5f);
	CHECK(seen == doctest::Approx(0.25f));
	t.SetEase(Easing::Linear);
	t.Stop();
	t.Start();
	t.Update(0.5f);
	CHECK(seen == doctest::Approx(0.5f));
}

TEST_CASE("Tween on_update Stop skips on_finish") {
	int finishes = 0;
	Tween t(1.f, true);
	t.SetOnUpdate([&](f32) { t.Stop(); });
	t.SetOnFinish([&]() { finishes++; });
	CHECK(t.Update(1.f) == false);
	CHECK(t.GetState() == Tween::State::Stopped);
	CHECK(finishes == 0);
}

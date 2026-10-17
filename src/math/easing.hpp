#pragma once

#include <cmath>
#include <numbers>

#include "core/types.hpp"

namespace sowa {

// Penner easing curves. Elastic and Back may overshoot [0, 1].
enum class Easing : i32 {
	Linear = 0,

	SineIn,
	SineOut,
	SineInOut,

	CubicIn,
	CubicOut,
	CubicInOut,

	QuintIn,
	QuintOut,
	QuintInOut,

	CircIn,
	CircOut,
	CircInOut,

	ElasticIn,
	ElasticOut,
	ElasticInOut,

	QuadIn,
	QuadOut,
	QuadInOut,

	QuartIn,
	QuartOut,
	QuartInOut,

	ExpoIn,
	ExpoOut,
	ExpoInOut,

	BackIn,
	BackOut,
	BackInOut,

	BounceIn,
	BounceOut,
	BounceInOut,

	Count,
};

namespace Math {

inline f32 Ease(f32 t, Easing easing) {
	constexpr f32 pi = std::numbers::pi_v<f32>;

	switch (easing) {
	case Easing::Linear:
		return t;
	case Easing::SineIn:
		return 1.f - std::cos((t * pi) * 0.5f);
	case Easing::SineOut:
		return std::sin((t * pi) * 0.5f);
	case Easing::SineInOut:
		return -(std::cos(pi * t) - 1.f) * 0.5f;
	case Easing::CubicIn:
		return t * t * t;
	case Easing::CubicOut: {
		const f32 u = 1.f - t;
		return 1.f - u * u * u;
	}
	case Easing::CubicInOut:
		if (t < 0.5f) {
			return 4.f * t * t * t;
		}
		{
			const f32 u = -2.f * t + 2.f;
			return 1.f - u * u * u * 0.5f;
		}
	case Easing::QuintIn:
		return t * t * t * t * t;
	case Easing::QuintOut:
		return 1.f - std::pow(1.f - t, 5.f);
	case Easing::QuintInOut:
		if (t < 0.5f) {
			return 16.f * t * t * t * t * t;
		}
		return 1.f - std::pow(-2.f * t + 2.f, 5.f) * 0.5f;
	case Easing::CircIn:
		return 1.f - std::sqrt(1.f - t * t);
	case Easing::CircOut:
		return std::sqrt(1.f - (t - 1.f) * (t - 1.f));
	case Easing::CircInOut:
		if (t < 0.5f) {
			return (1.f - std::sqrt(1.f - (2.f * t) * (2.f * t))) * 0.5f;
		}
		return (std::sqrt(1.f - (-2.f * t + 2.f) * (-2.f * t + 2.f)) + 1.f) * 0.5f;
	case Easing::ElasticIn:
		if (t == 0.f) {
			return 0.f;
		}
		if (t == 1.f) {
			return 1.f;
		}
		return -std::pow(2.f, 10.f * t - 10.f) * std::sin((t * 10.f - 10.75f) * (2.f * pi) * 0.33f);
	case Easing::ElasticOut:
		if (t == 0.f) {
			return 0.f;
		}
		if (t == 1.f) {
			return 1.f;
		}
		return std::pow(2.f, -10.f * t) * std::sin((t * 10.f - 0.75f) * (2.f * pi) * 0.33f) + 1.f;
	case Easing::ElasticInOut:
		if (t == 0.f) {
			return 0.f;
		}
		if (t == 1.f) {
			return 1.f;
		}
		if (t < 0.5f) {
			return -(std::pow(2.f, 20.f * t - 10.f) *
					   std::sin((20.f * t - 11.125f) * (2.f * pi) / 4.5f)) *
				   0.5f;
		}
		return (std::pow(2.f, -20.f * t + 10.f) *
				   std::sin((20.f * t - 11.125f) * (2.f * pi) / 4.5f)) *
				   0.5f +
			   1.f;
	case Easing::QuadIn:
		return t * t;
	case Easing::QuadOut: {
		const f32 u = 1.f - t;
		return 1.f - u * u;
	}
	case Easing::QuadInOut:
		if (t < 0.5f) {
			return 2.f * t * t;
		}
		return 1.f - std::pow(-2.f * t + 2.f, 2.f) * 0.5f;
	case Easing::QuartIn:
		return t * t * t * t;
	case Easing::QuartOut:
		return 1.f - std::pow(1.f - t, 4.f);
	case Easing::QuartInOut:
		if (t < 0.5f) {
			return 8.f * t * t * t * t;
		}
		return 1.f - std::pow(-2.f * t + 2.f, 4.f) * 0.5f;
	case Easing::ExpoIn:
		if (t == 0.f) {
			return 0.f;
		}
		return std::pow(2.f, 10.f * t - 10.f);
	case Easing::ExpoOut:
		if (t == 1.f) {
			return 1.f;
		}
		return 1.f - std::pow(2.f, -10.f * t);
	case Easing::ExpoInOut:
		if (t == 0.f) {
			return 0.f;
		}
		if (t == 1.f) {
			return 1.f;
		}
		if (t < 0.5f) {
			return std::pow(2.f, 20.f * t - 10.f) * 0.5f;
		}
		return (2.f - std::pow(2.f, -20.f * t + 10.f)) * 0.5f;
	case Easing::BackIn: {
		constexpr f32 c1 = 1.70158f;
		constexpr f32 c3 = c1 + 1.f;
		return c3 * t * t * t - c1 * t * t;
	}
	case Easing::BackOut: {
		constexpr f32 c1 = 1.70158f;
		constexpr f32 c3 = c1 + 1.f;
		return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
	}
	case Easing::BackInOut: {
		constexpr f32 c1 = 1.70158f;
		constexpr f32 c2 = c1 * 1.525f;
		if (t < 0.5f) {
			return (std::pow(2.f * t, 2.f) * ((c2 + 1.f) * 2.f * t - c2)) * 0.5f;
		}
		return (std::pow(2.f * t - 2.f, 2.f) * ((c2 + 1.f) * (t * 2.f - 2.f) + c2) + 2.f) * 0.5f;
	}
	case Easing::BounceOut: {
		f32 scaledTime = t;
		if (scaledTime < (1.f / 2.75f)) {
			return 7.5625f * scaledTime * scaledTime;
		}
		if (scaledTime < (2.f / 2.75f)) {
			const f32 scaledTime2 = scaledTime - (1.5f / 2.75f);
			return 7.5625f * scaledTime2 * scaledTime2 + 0.75f;
		}
		if (scaledTime < (2.5f / 2.75f)) {
			const f32 scaledTime2 = scaledTime - (2.25f / 2.75f);
			return 7.5625f * scaledTime2 * scaledTime2 + 0.9375f;
		}
		const f32 scaledTime2 = scaledTime - (2.625f / 2.75f);
		return 7.5625f * scaledTime2 * scaledTime2 + 0.984375f;
	}
	case Easing::BounceIn:
		return 1.f - Ease(1.f - t, Easing::BounceOut);
	case Easing::BounceInOut:
		if (t < 0.5f) {
			return (1.f - Ease(1.f - 2.f * t, Easing::BounceOut)) * 0.5f;
		}
		return (1.f + Ease(2.f * t - 1.f, Easing::BounceOut)) * 0.5f;
	case Easing::Count:
		break;
	}
	return t;
}

} // namespace Math
} // namespace sowa

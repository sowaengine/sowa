#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

#include "core/types.hpp"
#include "math/color.hpp"
#include "math/easing.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace sowa::Math {

constexpr f32 Pi = std::numbers::pi_v<f32>;

inline f32 Abs(f32 value) {
	return std::fabs(value);
}

inline f32 Sin(f32 value) {
	return std::sin(value);
}

inline f32 Cos(f32 value) {
	return std::cos(value);
}

inline f32 Sqrt(f32 value) {
	return std::sqrt(value);
}

template <typename T> inline T Clamp(T value, T min, T max) {
	if (min > max) {
		std::swap(min, max);
	}
	value = std::max(value, min);
	value = std::min(value, max);
	return value;
}

template <typename T> inline T Lerp(T start, T end, f32 t) {
	return start + ((end - start) * t);
}

template <typename T> inline T Lerp(T start, T end, f32 t, Easing easing) {
	return Lerp(start, end, Ease(t, easing));
}

template <> inline Vector2 Lerp(Vector2 start, Vector2 end, f32 t) {
	return Vector2(Lerp(start.x, end.x, t), Lerp(start.y, end.y, t));
}

template <> inline Vector3 Lerp(Vector3 start, Vector3 end, f32 t) {
	return Vector3(Lerp(start.x, end.x, t), Lerp(start.y, end.y, t), Lerp(start.z, end.z, t));
}

template <> inline Color Lerp(Color start, Color end, f32 t) {
	return Color(Lerp(start.r, end.r, t), Lerp(start.g, end.g, t), Lerp(start.b, end.b, t),
		Lerp(start.a, end.a, t));
}

inline f32 Atan2(f32 y, f32 x) {
	return std::atan2(y, x);
}

template <typename T> inline T Map(T v, T inputMin, T inputMax, T outputMin, T outputMax) {
	return (v - inputMin) * (outputMax - outputMin) / (inputMax - inputMin) + outputMin;
}

// Shortcut for Map with input range 0..1.
template <typename T> inline T TweenMap(T v, T outputMin, T outputMax) {
	return Map(v, static_cast<T>(0), static_cast<T>(1), outputMin, outputMax);
}

} // namespace sowa::Math

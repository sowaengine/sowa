#pragma once

#include <cmath>

#include "core/types.hpp"

namespace sowa {

class Vector2 {
  public:
	Vector2() : x(0.f), y(0.f) {
	}

	Vector2(f32 v) : x(v), y(v) {
	}

	Vector2(f32 x, f32 y) : x(x), y(y) {
	}

	f32 x;
	f32 y;

	f32 Length() const {
		return std::sqrt(x * x + y * y);
	}

	f32 LengthSquared() const {
		return x * x + y * y;
	}

	f32 DistanceTo(const Vector2 &point) const {
		return (point - *this).Length();
	}

	void Normalize() {
		f32 len = Length();
		if (len == 0.f) {
			return;
		}

		x = x / len;
		y = y / len;
	}

	Vector2 Normalized() const {
		Vector2 vec(*this);
		vec.Normalize();
		return vec;
	}

	Vector2 operator-() const {
		return Vector2(-x, -y);
	}

	Vector2 operator-(const Vector2 &obj) const {
		return Vector2(x - obj.x, y - obj.y);
	}

	Vector2 &operator-=(const Vector2 &obj) {
		x -= obj.x;
		y -= obj.y;
		return *this;
	}

	Vector2 operator+(const Vector2 &obj) const {
		return Vector2(x + obj.x, y + obj.y);
	}

	Vector2 &operator+=(const Vector2 &obj) {
		x += obj.x;
		y += obj.y;
		return *this;
	}

	Vector2 operator*(const Vector2 &obj) const {
		return Vector2(x * obj.x, y * obj.y);
	}

	Vector2 &operator*=(const Vector2 &obj) {
		x *= obj.x;
		y *= obj.y;
		return *this;
	}

	Vector2 operator*(f32 v) const {
		return Vector2(x * v, y * v);
	}

	Vector2 &operator*=(f32 v) {
		x *= v;
		y *= v;
		return *this;
	}

	Vector2 operator/(const Vector2 &obj) const {
		return Vector2(x / obj.x, y / obj.y);
	}

	Vector2 &operator/=(const Vector2 &obj) {
		x /= obj.x;
		y /= obj.y;
		return *this;
	}

	Vector2 operator/(f32 v) const {
		return Vector2(x / v, y / v);
	}

	Vector2 &operator/=(f32 v) {
		x /= v;
		y /= v;
		return *this;
	}

	bool operator==(const Vector2 &obj) const {
		return x == obj.x && y == obj.y;
	}

	bool operator!=(const Vector2 &obj) const {
		return !(*this == obj);
	}
};

} // namespace sowa

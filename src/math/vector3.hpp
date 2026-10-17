#pragma once

#include <cmath>

#include "core/types.hpp"

namespace sowa {

class Vector3 {
  public:
	Vector3() : x(0.f), y(0.f), z(0.f) {
	}

	Vector3(f32 v) : x(v), y(v), z(v) {
	}

	Vector3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {
	}

	f32 x;
	f32 y;
	f32 z;

	f32 Length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	f32 LengthSquared() const {
		return x * x + y * y + z * z;
	}

	f32 DistanceTo(const Vector3 &point) const {
		return (point - *this).Length();
	}

	void Normalize() {
		f32 len = Length();
		if (len == 0.f) {
			return;
		}

		x = x / len;
		y = y / len;
		z = z / len;
	}

	Vector3 Normalized() const {
		Vector3 vec(*this);
		vec.Normalize();
		return vec;
	}

	Vector3 operator-() const {
		return Vector3(-x, -y, -z);
	}

	Vector3 operator-(const Vector3 &obj) const {
		return Vector3(x - obj.x, y - obj.y, z - obj.z);
	}

	Vector3 &operator-=(const Vector3 &obj) {
		x -= obj.x;
		y -= obj.y;
		z -= obj.z;
		return *this;
	}

	Vector3 operator+(const Vector3 &obj) const {
		return Vector3(x + obj.x, y + obj.y, z + obj.z);
	}

	Vector3 &operator+=(const Vector3 &obj) {
		x += obj.x;
		y += obj.y;
		z += obj.z;
		return *this;
	}

	Vector3 operator*(const Vector3 &obj) const {
		return Vector3(x * obj.x, y * obj.y, z * obj.z);
	}

	Vector3 &operator*=(const Vector3 &obj) {
		x *= obj.x;
		y *= obj.y;
		z *= obj.z;
		return *this;
	}

	Vector3 operator*(f32 v) const {
		return Vector3(x * v, y * v, z * v);
	}

	Vector3 &operator*=(f32 v) {
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	Vector3 operator/(const Vector3 &obj) const {
		return Vector3(x / obj.x, y / obj.y, z / obj.z);
	}

	Vector3 &operator/=(const Vector3 &obj) {
		x /= obj.x;
		y /= obj.y;
		z /= obj.z;
		return *this;
	}

	Vector3 operator/(f32 v) const {
		return Vector3(x / v, y / v, z / v);
	}

	Vector3 &operator/=(f32 v) {
		x /= v;
		y /= v;
		z /= v;
		return *this;
	}

	bool operator==(const Vector3 &obj) const {
		return x == obj.x && y == obj.y && z == obj.z;
	}

	bool operator!=(const Vector3 &obj) const {
		return !(*this == obj);
	}
};

} // namespace sowa

#ifndef SW_VEC2_HXX
#define SW_VEC2_HXX
#pragma once

#include "glm/glm.hpp"
#include <initializer_list>
#include <iostream>

struct vec2 {
	float x;
	float y;

	vec2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	vec2(float s) {
		this->x = s;
		this->y = s;
	}

	vec2() {
		this->x = 0.f;
		this->y = 0.f;
	}

	vec2(const vec2 &v) {
		this->x = v.x;
		this->y = v.y;
	}

	vec2(const glm::vec2 &v) {
		this->x = v.x;
		this->y = v.y;
	}

	vec2 operator*(float s) {
		return vec2(this->x * s, this->y * s);
	}

	vec2 operator*(const vec2 &v) {
		return vec2(this->x * v.x, this->y * v.y);
	}

	vec2 operator+(const vec2 &v) {
		return vec2(this->x + v.x, this->y + v.y);
	}

	vec2 operator+=(const vec2 &v) {
		this->x += v.x;
		this->y += v.y;
		return *this;
	}

	vec2 operator*=(const vec2 &v) {
		this->x *= v.x;
		this->y *= v.y;
		return *this;
	}

	vec2 operator*=(float s) {
		this->x *= s;
		this->y *= s;
		return *this;
	}

	vec2 operator-(const vec2 &v) {
		return vec2(this->x - v.x, this->y - v.y);
	}

	bool operator==(const vec2 &other) {
		return this->x == other.x && this->y == other.y;
	}

	vec2 &operator=(const vec2 &other) {
		this->x = other.x;
		this->y = other.y;
		return *this;
	}

	vec2 &operator=(const glm::vec2 &other) {
		this->x = other.x;
		this->y = other.y;
		return *this;
	}

	vec2 &operator=(const std::initializer_list<float> &v) {
		if (v.size() == 1) {
			this->x = *v.begin();
			this->y = *v.begin();
		} else if (v.size() == 2) {
			this->x = *v.begin();
			this->y = *(v.begin() + 1);
		}
		return *this;
	}

	vec2 &operator=(const std::initializer_list<int> &v) {
		if (v.size() == 1) {
			this->x = static_cast<float>(*v.begin());
			this->y = static_cast<float>(*v.begin());
		} else if (v.size() == 2) {
			this->x = static_cast<float>(*v.begin());
			this->y = static_cast<float>(*(v.begin() + 1));
		}
		return *this;
	}
};

#endif // SW_VEC2_HXX
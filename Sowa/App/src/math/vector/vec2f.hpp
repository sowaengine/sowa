#ifndef SW_VEC2F_HPP_
#define SW_VEC2F_HPP_

#include <string>

#include "glm/glm.hpp"

#include "object_type.hpp"
#include "serialize/serializer.hpp"

namespace sowa {
struct vec2f : public object_type {
	float x;
	float y;

	static std::string Typename() { return "sowa::vec2f"; }

	vec2f(float x_, float y_);
	vec2f();

	vec2f &operator=(const vec2f &rhs);
	vec2f &operator=(const glm::vec2 &rhs);

	operator glm::vec2();

	static FileBuffer SaveImpl(object_type *obj);

	static bool LoadImpl(object_type *out, const FileBuffer &buf);

	vec2f &operator=(std::initializer_list<float> rhs);
	vec2f &operator=(std::initializer_list<double> rhs);
	vec2f &operator=(std::initializer_list<int> rhs);

	/// @brief Returns magnitude of vector
	float length();

	/// @brief returns a vector with same direction with magnitude @param length
	vec2f clamp(float length = 1.0f);

	/// @brief returns angle of vector in radians starting from right and rotates clockwise. (1, 0) -> 0 and (0, 1) -> PI / 2
	float angle();
};
} // namespace sowa

#endif // SW_VEC2F_HPP_
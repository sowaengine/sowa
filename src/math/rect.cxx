#include "rect.hxx"
#include "utils/utils.hxx"
#include "vec2.hxx"

vec2 rect::map_point(const vec2 &point, const rect &target) {
	return vec2(Utils::Map(point.x, target.x, target.x + target.w, x, x + w), Utils::Map(point.y, target.y, target.y + target.h, y, y + h));
}
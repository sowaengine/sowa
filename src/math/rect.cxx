#include "rect.hxx"
#include "utils/utils.hxx"
#include "vec2.hxx"

vec2 rect::map_point(const vec2 &point, const rect &target) {
	return vec2(Utils::Map(point.x, this->x, this->x + this->w, target.x, target.x + target.w), Utils::Map(point.y, this->y, this->y + this->h, target.y, target.y + target.h));
}

bool rect::is_point_in(const vec2 &point) {
	return point.x > x && point.x < x + w && point.y > y && point.y < y + h;
}
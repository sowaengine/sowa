#include "rect.hpp"

#include "utils/algorithm.hpp"

namespace sowa {
rect::rect() : x(0.f), y(0.f), w(0.f), h(0.f) {
}
rect::rect(float w_, float h_) : x(0.f), y(0.f), w(w_), h(h_) {
}
rect::rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {
}

vec2 rect::mapPoint(const vec2& point, const rect& target) {
    return vec2(map(point.x, target.x, target.x + target.w, x, x + w), map(point.y, target.y, target.y + target.h, y, y + h));
}
} // namespace sowa

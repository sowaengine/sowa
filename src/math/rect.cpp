#include "math/rect.hpp"

#include "math/math.hpp"
#include "math/vector2.hpp"

namespace sowa {

Vector2 Rect::MapPoint(const Vector2 &point, const Rect &target) const {
	return Vector2(Math::Map(point.x, x, x + w, target.x, target.x + target.w),
		Math::Map(point.y, y, y + h, target.y, target.y + target.h));
}

bool Rect::Contains(const Vector2 &point) const {
	return point.x >= Left() && point.x <= Right() && point.y >= Bottom() && point.y <= Top();
}

bool Rect::Intersects(const Rect &other) const {
	return Left() <= other.Right() && Right() >= other.Left() && Bottom() <= other.Top() &&
		   Top() >= other.Bottom();
}

} // namespace sowa

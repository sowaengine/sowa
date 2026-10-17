#include "ecs/components/parallax2d.hpp"

#include "ecs/world.hpp"

namespace sowa {

void RegisterParallax2D(World &world) {
	world.RegisterComponent<Parallax2D>("Parallax2D")
		.Field("tileX", &Parallax2D::tileX)
		.Field("tileY", &Parallax2D::tileY)
		.Field("xSpeed", &Parallax2D::xSpeed)
		.Field("ySpeed", &Parallax2D::ySpeed)
		.Field("offset", &Parallax2D::offset)
		.Field("limitX", &Parallax2D::limitX)
		.Field("limitY", &Parallax2D::limitY)
		.Field("limitMin", &Parallax2D::limitMin)
		.Field("limitMax", &Parallax2D::limitMax);
}

} // namespace sowa

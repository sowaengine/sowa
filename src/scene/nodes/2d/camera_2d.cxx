#include "camera_2d.hxx"

#include "core/tweens.hxx"
#include "scene/scene_tree.hxx"

#include "core/app.hxx"

void Camera2D::_update() {
	if (!App::get().IsRunning()) {
		rect bounds = get_bounds();
		const float thickness = 6.f;
		const float r = 0.56f;
		const float g = 0.2f;
		const float b = 0.92f;
		const float z = 100.f;
		App::get().Renderer().PushLine(vec2(bounds.x, bounds.y), vec2(bounds.x + bounds.w, bounds.y), thickness, r, g, b, 1.f, z);
		App::get().Renderer().PushLine(vec2(bounds.x, bounds.y), vec2(bounds.x, bounds.y + bounds.h), thickness, r, g, b, 1.f, z);
		App::get().Renderer().PushLine(vec2(bounds.x + bounds.w, bounds.y + bounds.h), vec2(bounds.x, bounds.y + bounds.h), thickness, r, g, b, 1.f, z);
		App::get().Renderer().PushLine(vec2(bounds.x + bounds.w, bounds.y), vec2(bounds.x + bounds.w, bounds.y + bounds.h), thickness, r, g, b, 1.f, z);
	}
}

rect Camera2D::get_bounds() {
	vec2 pos = global_position();
	vec2 center = center_point();
	vec2 size(1920.f, 1080.f);
	size *= zoom();

	rect r;
	r.x = pos.x - (size.x * center.x);
	r.y = pos.y - (size.y * center.y);
	r.w = size.x;
	r.h = size.y;

	return r;
}
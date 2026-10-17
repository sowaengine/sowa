#pragma once

#include <flecs.h>

namespace sowa {

// Render visibility tag. Visible-only queries exclude the entity and all descendants
// without requiring visibility components on children. Hidden entities still simulate;
// use enabled state to control processing.
struct Hidden {};

// Whether a query should skip hidden entities.
enum class VisibilityFilter {
	// Every match, hidden or not. The default, because most queries (physics,
	// gameplay, the editor's entity list) have no business caring.
	All,
	// Only entities that are neither hidden themselves nor below something
	// hidden. What drawing wants.
	VisibleOnly,
};

// Adds VisibleOnly query terms to exclude Hidden on the entity or any ancestor.
template <typename Builder> Builder &ApplyVisibleOnly(Builder &builder) {
	builder.template with<Hidden>().not_();
	builder.template with<Hidden>().parent().not_();
	return builder;
}

} // namespace sowa

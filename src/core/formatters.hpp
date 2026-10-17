#pragma once

// {fmt} specializations for engine value types. Included by log/string so
// Log::Info("{}", value) and String::Format("{}", value) work without callers
// having to remember an extra include.
#include <fmt/format.h>

#include "core/types.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

template <> struct fmt::formatter<sowa::Color> {
	constexpr auto parse(format_parse_context &ctx) {
		return ctx.begin();
	}

	template <typename Context> auto format(const sowa::Color &value, Context &ctx) const {
		return format_to(ctx.out(), "Color({}, {}, {}, {})", static_cast<sowa::u8>(value.r * 255.f),
			static_cast<sowa::u8>(value.g * 255.f), static_cast<sowa::u8>(value.b * 255.f),
			static_cast<sowa::u8>(value.a * 255.f));
	}
};

template <> struct fmt::formatter<sowa::Vector2> {
	constexpr auto parse(format_parse_context &ctx) {
		return ctx.begin();
	}

	template <typename Context> auto format(const sowa::Vector2 &value, Context &ctx) const {
		return format_to(ctx.out(), "Vector2({}, {})", value.x, value.y);
	}
};

template <> struct fmt::formatter<sowa::Vector3> {
	constexpr auto parse(format_parse_context &ctx) {
		return ctx.begin();
	}

	template <typename Context> auto format(const sowa::Vector3 &value, Context &ctx) const {
		return format_to(ctx.out(), "Vector3({}, {}, {})", value.x, value.y, value.z);
	}
};

template <> struct fmt::formatter<sowa::Rect> {
	constexpr auto parse(format_parse_context &ctx) {
		return ctx.begin();
	}

	template <typename Context> auto format(const sowa::Rect &value, Context &ctx) const {
		return format_to(ctx.out(), "Rect({}, {}, {}, {})", value.x, value.y, value.w, value.h);
	}
};
